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
from typing import Dict, Iterable, Optional

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


def package_environment(
    env: str,
    chip: str,
    build_dir: Path,
    channel: str,
    version: str,
    output_dir: Path,
    published_at: str,
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


def write_index(output_dir: Path, summary: Iterable[Dict[str, str]], published_at: str, version: str) -> None:
    summary_list = [item for item in summary if item]
    if not summary_list:
        return

    index_path = output_dir / "firmware" / "index.json"
    index_path.parent.mkdir(parents=True, exist_ok=True)
    index = {
        "generated_at": published_at,
        "version": version,
        "artifacts": summary_list,
    }
    index_path.write_text(json.dumps(index, indent=2))


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
        )
        if summary:
            summaries.append(summary)

    write_index(args.output, summaries, published_at, version)


if __name__ == "__main__":
    main()
