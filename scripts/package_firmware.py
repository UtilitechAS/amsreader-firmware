#!/usr/bin/env python3
"""
Package compiled PlatformIO firmware artifacts into a static layout suitable for
publishing on GitHub Pages (or any static web host). For each target environment
we copy the generated firmware binary, compute its checksum, and emit a manifest
JSON document that the device firmware can consume when looking for OTA updates.

Example structure produced under the output directory:

    dist/
      firmware/
        esp32s2/
          stable/
            firmware-abcdef1.bin
            manifest.json

The manifest contains version, size, checksum, and download path information.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
from datetime import datetime, timezone
from typing import Any, Dict, Iterable, List, Optional, Tuple

DEFAULT_CHANNEL = "stable"
DEFAULT_OUTPUT = Path("dist")

# Mapping from PlatformIO environment names to chip identifiers used by the OTA
# client. Update this map if new environments are added to platformio.ini.
ENV_TO_CHIP: Dict[str, str] = {
    "esp8266": "esp8266",
    "esp32": "esp32",
    "esp32s2": "esp32s2",
    "esp32s3": "esp32s3",
    "esp32c3": "esp32c3",
    "esp32solo": "esp32solo",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Package Firmware Artifacts")
    parser.add_argument(
        "--env",
        dest="envs",
        action="append",
        help="PlatformIO environment(s) to package (defaults to all known)",
    )
    parser.add_argument(
        "--channel",
        default=DEFAULT_CHANNEL,
        help="Firmware update channel name (default: stable)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Destination directory for packaged artifacts (default: dist)",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=Path(".pio") / "build",
        help="Base directory containing PlatformIO build outputs",
    )
    parser.add_argument(
        "--version",
        type=str,
        default=None,
        help="Firmware version string (falls back to generated_version.h)",
    )
    parser.add_argument(
        "--published-at",
        type=str,
        default=None,
        help="ISO timestamp for the manifest (default: current UTC time)",
    )
    return parser.parse_args()


def read_version(version_override: Optional[str]) -> str:
    if version_override:
        return version_override.strip()

    header_path = Path("lib/FirmwareVersion/src/generated_version.h")
    if not header_path.exists():
        raise FileNotFoundError(
            f"generated_version.h not found at {header_path}. Run PlatformIO build first."
        )

    for line in header_path.read_text().splitlines():
        if "VERSION_STRING" in line:
            parts = line.split('"')
            if len(parts) >= 2:
                return parts[1]
    raise RuntimeError("Unable to extract VERSION_STRING from generated header")


def compute_md5(path: Path) -> str:
    hash_md5 = hashlib.md5()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


MQTT_FIELD_SPECS: Tuple[Tuple[str, str, type], ...] = (
    ("host", "MQTT_DEFAULT_HOST", str),
    ("port", "MQTT_DEFAULT_PORT", int),
    ("username", "MQTT_DEFAULT_USERNAME", str),
    ("password", "MQTT_DEFAULT_PASSWORD", str),
    ("client_id", "MQTT_DEFAULT_CLIENT_ID", str),
    ("publish_topic", "MQTT_DEFAULT_PUBLISH_TOPIC", str),
    ("subscribe_topic", "MQTT_DEFAULT_SUBSCRIBE_TOPIC", str),
    ("payload_format", "MQTT_DEFAULT_PAYLOAD_FORMAT", int),
    ("ssl", "MQTT_DEFAULT_SSL", bool),
    ("state_update", "MQTT_DEFAULT_STATE_UPDATE", bool),
    ("state_update_interval", "MQTT_DEFAULT_STATE_UPDATE_INTERVAL", int),
    ("timeout", "MQTT_DEFAULT_TIMEOUT", int),
    ("keepalive", "MQTT_DEFAULT_KEEPALIVE", int),
)


def _parse_env_file(path: Path) -> Dict[str, str]:
    data: Dict[str, str] = {}
    if not path.exists():
        return data
    with path.open("r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            key, sep, value = line.partition("=")
            if not sep:
                continue
            key = key.strip()
            value = value.strip()
            if len(value) >= 2 and value[0] == value[-1] and value[0] in {'"', "'"}:
                value = value[1:-1]
            data[key] = value
    return data


def _to_bool(value: str) -> bool:
    truthy = {"1", "true", "yes", "on"}
    falsy = {"0", "false", "no", "off"}
    lowered = value.lower()
    if lowered in truthy:
        return True
    if lowered in falsy:
        return False
    raise ValueError(f"Invalid boolean literal: {value}")


def load_mqtt_defaults(project_dir: Path) -> Dict[str, Any]:
    env_path = project_dir / ".env"
    file_values = _parse_env_file(env_path)
    manifest_values: Dict[str, Any] = {}

    for manifest_key, env_key, value_type in MQTT_FIELD_SPECS:
        raw_value = os.getenv(env_key)
        if raw_value is None:
            raw_value = file_values.get(env_key)
        if raw_value is None or raw_value == "":
            continue

        try:
            if value_type is bool:
                converted: Any = _to_bool(raw_value)
            elif value_type is int:
                converted = int(raw_value, 0)
            else:
                converted = raw_value
        except ValueError as exc:
            print(f"WARN: Skipping MQTT field {env_key}: {exc}")
            continue

        manifest_values[manifest_key] = converted

    return manifest_values


def package_environment(
    env: str,
    chip: str,
    build_dir: Path,
    channel: str,
    version: str,
    output_dir: Path,
    published_at: str,
    mqtt_defaults: Optional[Dict[str, Any]] = None,
) -> Optional[Dict[str, str]]:
    firmware_path = build_dir / env / "firmware.bin"
    if not firmware_path.exists():
        print(f"WARN: Skipping {env}, firmware not found at {firmware_path}")
        return None

    md5_digest = compute_md5(firmware_path)
    size_bytes = firmware_path.stat().st_size

    channel_dir = output_dir / "firmware" / chip / channel
    channel_dir.mkdir(parents=True, exist_ok=True)

    binary_name = f"{chip}-{version}.bin"
    binary_dest = channel_dir / binary_name
    # Copy firmware binary (overwrite if it already exists)
    binary_dest.write_bytes(firmware_path.read_bytes())

    manifest_path = channel_dir / "manifest.json"
    manifest = {
        "version": version,
        "channel": channel,
        "chip": chip,
        "size": size_bytes,
        "md5": md5_digest,
        "url": binary_name,
        "published_at": published_at,
        "env": env,
    }
    if mqtt_defaults:
        manifest["mqtt"] = mqtt_defaults
    manifest_path.write_text(json.dumps(manifest, indent=2))

    # Optional: bundle flashing zip if produced by scripts/mkzip.sh
    zip_source = Path(f"{env}.zip")
    zip_dest = None
    if zip_source.exists():
        releases_dir = output_dir / "releases"
        releases_dir.mkdir(parents=True, exist_ok=True)
        zip_dest = releases_dir / f"{chip}-{version}.zip"
        zip_dest.write_bytes(zip_source.read_bytes())

    return {
        "env": env,
        "chip": chip,
        "channel": channel,
        "manifest": manifest_path.relative_to(output_dir).as_posix(),
        "binary": binary_dest.relative_to(output_dir).as_posix(),
        "zip": zip_dest.relative_to(output_dir).as_posix() if zip_dest else None,
        "md5": md5_digest,
        "size": size_bytes,
    }


def write_index(output_dir: Path, summary: Iterable[Dict[str, str]], published_at: str, version: str) -> List[Dict[str, str]]:
    summary_list = [item for item in summary if item]
    if summary_list:
        index_path = output_dir / "firmware" / "index.json"
        index_path.parent.mkdir(parents=True, exist_ok=True)
        index = {
            "generated_at": published_at,
            "version": version,
            "artifacts": summary_list,
        }
        index_path.write_text(json.dumps(index, indent=2))
    return summary_list


def write_root_index(output_dir: Path, artifacts: List[Dict[str, str]], channel: str, version: str, published_at: str) -> None:
    index_path = output_dir / "index.html"
    rows = []
    for artifact in artifacts:
        manifest_href = artifact.get("manifest")
        binary_href = artifact.get("binary")
        zip_href = artifact.get("zip")
        chip = artifact.get("chip", "?")
        env = artifact.get("env", "?")
        row = f"<tr><td>{chip}</td><td>{env}</td><td><a href='{manifest_href}'>manifest</a></td>"
        if binary_href:
            row += f"<td><a href='{binary_href}'>binary</a></td>"
        else:
            row += "<td>n/a</td>"
        if zip_href:
            row += f"<td><a href='{zip_href}'>zip</a></td>"
        else:
            row += "<td>n/a</td>"
        row += "</tr>"
        rows.append(row)

    table_rows = "\n".join(rows) if rows else "<tr><td colspan='5'>No firmware artifacts generated.</td></tr>"

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>Firmware packages {version}</title>
  <style>
    body {{ font-family: system-ui, sans-serif; margin: 2rem; }}
    table {{ border-collapse: collapse; min-width: 60%; }}
    th, td {{ border: 1px solid #ccc; padding: 0.5rem 0.75rem; text-align: left; }}
    th {{ background: #f5f5f5; }}
  </style>
</head>
<body>
  <h1>Firmware packages ({version})</h1>
  <p>Channel: <strong>{channel}</strong> &middot; Published at: <strong>{published_at}</strong></p>
  <p>The table below lists all firmware bundles generated by the release workflow. Devices should download the manifest matching their chip.</p>
  <table>
    <thead>
      <tr><th>Chip</th><th>Environment</th><th>Manifest</th><th>Binary</th><th>Zip</th></tr>
    </thead>
    <tbody>
      {table_rows}
    </tbody>
  </table>
</body>
</html>
"""
    index_path.write_text(html)


def main() -> None:
    args = parse_args()
    envs = args.envs if args.envs else list(ENV_TO_CHIP.keys())

    unknown_envs = [env for env in envs if env not in ENV_TO_CHIP]
    if unknown_envs:
        raise ValueError(f"Unknown PlatformIO environment(s): {', '.join(unknown_envs)}")

    version = read_version(args.version)
    published_at = (
        args.published_at
        if args.published_at
        else datetime.now(timezone.utc).isoformat(timespec="seconds")
    )

    project_dir = Path.cwd()
    mqtt_defaults = load_mqtt_defaults(project_dir)
    if mqtt_defaults:
        print("Including MQTT defaults in manifest for release packaging")

    summaries = []
    for env in envs:
        chip = ENV_TO_CHIP[env]
        summary = package_environment(
            env=env,
            chip=chip,
            build_dir=args.build_dir,
            channel=args.channel,
            version=version,
            output_dir=args.output,
            published_at=published_at,
            mqtt_defaults=mqtt_defaults,
        )
        if summary:
            summaries.append(summary)

    artifacts = write_index(args.output, summaries, published_at, version)
    write_root_index(args.output, artifacts, args.channel, version, published_at)


if __name__ == "__main__":
    main()
