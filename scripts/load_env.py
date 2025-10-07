import os
from typing import Any, Dict, Optional

from SCons.Script import Import

Import("env")
build_env = env  # type: ignore[name-defined]

PROJECT_DIR = build_env["PROJECT_DIR"]
ENV_PATH = os.path.join(PROJECT_DIR, ".env")

STRING_KEYS = {
    "MQTT_DEFAULT_HOST",
    "MQTT_DEFAULT_USERNAME",
    "MQTT_DEFAULT_PASSWORD",
    "MQTT_DEFAULT_CLIENT_ID",
    "MQTT_DEFAULT_PUBLISH_TOPIC",
    "MQTT_DEFAULT_SUBSCRIBE_TOPIC",
}

NUMERIC_KEYS = {
    "MQTT_DEFAULT_PORT",
    "MQTT_DEFAULT_PAYLOAD_FORMAT",
    "MQTT_DEFAULT_STATE_UPDATE_INTERVAL",
    "MQTT_DEFAULT_TIMEOUT",
    "MQTT_DEFAULT_KEEPALIVE",
}

BOOL_KEYS = {
    "MQTT_DEFAULT_SSL",
    "MQTT_DEFAULT_STATE_UPDATE",
}


def _strip_quotes(value: str) -> str:
    if len(value) >= 2 and value[0] == value[-1] and value[0] in {'"', '\''}:
        return value[1:-1]
    return value


def _parse_env_file(path: str) -> Dict[str, str]:
    data: Dict[str, str] = {}
    with open(path, "r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            key, sep, value = line.partition("=")
            if not sep:
                continue
            key = key.strip()
            value = _strip_quotes(value.strip())
            data[key] = value
    return data


def _append_define(name: str, value: str) -> Optional[Any]:
    if name in STRING_KEYS:
        sanitized = value.replace('"', '\\"')
        define = (name, f'\\"{sanitized}\\"')
        build_env.Append(CPPDEFINES=[define])
        return value
    elif name in NUMERIC_KEYS:
        try:
            number = int(value, 0)
            define = (name, number)
            build_env.Append(CPPDEFINES=[define])
            return number
        except ValueError as exc:  # noqa: F841 - retain context for debugging
            raise ValueError(f"Invalid integer for {name}: '{value}'")
    elif name in BOOL_KEYS:
        truthy = {"1", "true", "yes", "on"}
        falsy = {"0", "false", "no", "off"}
        lowered = value.lower()
        if lowered in truthy:
            define = (name, 1)
            build_env.Append(CPPDEFINES=[define])
            return True
        elif lowered in falsy:
            define = (name, 0)
            build_env.Append(CPPDEFINES=[define])
            return False
        else:
            raise ValueError(f"Invalid boolean for {name}: '{value}'")
    else:
        # Allow additional CPP macro overrides without hard-coding the type
        sanitized = value.replace('"', '\\"')
        define = (name, f'\\"{sanitized}\\"')
        build_env.Append(CPPDEFINES=[define])
        return value
    return None


if os.path.exists(ENV_PATH):
    values = _parse_env_file(ENV_PATH)
    applied_defines = []
    if values:
        print("[load_env] Injecting MQTT defaults from .env:")
        for debug_key, debug_value in values.items():
            print(f"  {debug_key}={debug_value}")
    for key, raw_value in values.items():
        if raw_value == "":
            continue
        typed_value = _append_define(key, raw_value)
        if typed_value is not None:
            applied_defines.append((key, typed_value))
    if applied_defines:
        print("[load_env] Resulting CPPDEFINES tail:")
        for name, val in applied_defines:
            if name in STRING_KEYS or isinstance(val, str):
                print(f"  {name}=\"{val}\"")
            elif name in BOOL_KEYS:
                print(f"  {name}={1 if val else 0}")
            else:
                print(f"  {name}={val}")
