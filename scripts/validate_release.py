from pathlib import Path
import hashlib
import json
import sys
import zipfile


PROJECT_DIR = Path(__file__).resolve().parents[1]
DIST_DIR = PROJECT_DIR / "dist"
SITE_DIR = DIST_DIR / "browser-installer"
RELEASE_DIR = DIST_DIR / "release-package"


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    errors = []
    manifest_path = SITE_DIR / "manifest.json"
    build_info_path = SITE_DIR / "build-info.json"

    for required_path in (
        SITE_DIR / "index.html",
        manifest_path,
        build_info_path,
        SITE_DIR / "downloads" / "zehnder-comfoair-dashboard.zip",
        SITE_DIR / "downloads" / "zehnder-comfoair-dashboard.sha256",
        RELEASE_DIR / "SHA256SUMS.txt",
        RELEASE_DIR / "SOURCE.txt",
        RELEASE_DIR / "LICENSE",
        RELEASE_DIR / "THIRD_PARTY_NOTICES.md",
        RELEASE_DIR / "home-assistant" / "README.md",
        RELEASE_DIR / "home-assistant" / "dashboard-native.yaml",
        RELEASE_DIR / "home-assistant" / "dashboard-animated.yaml",
        RELEASE_DIR / "home-assistant" / "view-native.yaml",
        RELEASE_DIR / "home-assistant" / "view-animated.yaml",
        RELEASE_DIR / "home-assistant" / "view-custom-card.yaml",
        (
            RELEASE_DIR
            / "home-assistant"
            / "custom-card"
            / "gerlcraft-zehnder-card.js"
        ),
        RELEASE_DIR / "home-assistant" / "custom-card" / "INSTALLATION.md",
        (
            RELEASE_DIR
            / "home-assistant"
            / "node-red"
            / "zehnder-bad-feuchte-boost.json"
        ),
    ):
        if not required_path.is_file():
            errors.append(f"Datei fehlt: {required_path}")

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    build_info = json.loads(build_info_path.read_text(encoding="utf-8"))
    builds = manifest.get("builds", [])

    if len(builds) != 1 or builds[0].get("chipFamily") != "ESP32-S3":
        errors.append("Manifest muss genau einen ESP32-S3-Build enthalten")

    parts = builds[0].get("parts", []) if builds else []

    if len(parts) != 1 or parts[0].get("offset") != 0:
        errors.append("Manifest muss eine Factory-Firmware an Offset 0 nutzen")
    else:
        factory_path = SITE_DIR / parts[0].get("path", "")

        if not factory_path.is_file():
            errors.append(f"Factory-Firmware fehlt: {factory_path}")
        else:
            if factory_path.stat().st_size >= 8 * 1024 * 1024:
                errors.append("Factory-Firmware ist groesser als der Flash")

            if factory_path.read_bytes()[:1] != b"\xe9":
                errors.append("Factory-Firmware hat keinen ESP-Image-Header")

            expected_hash = build_info.get("artifacts", {}).get(
                factory_path.name
            )

            if expected_hash != sha256(factory_path):
                errors.append("SHA-256 der Factory-Firmware stimmt nicht")

    archives = list(DIST_DIR.glob("zehnder-comfoair-bridge-v*.zip"))

    if len(archives) != 1:
        errors.append("Genau ein Release-ZIP wurde erwartet")
    else:
        with zipfile.ZipFile(archives[0], "r") as archive:
            bad_file = archive.testzip()

            if bad_file is not None:
                errors.append(f"Defekte ZIP-Datei: {bad_file}")

            names = set(archive.namelist())

            for required_name in (
                "LICENSE",
                "THIRD_PARTY_NOTICES.md",
                "SOURCE.txt",
                "SHA256SUMS.txt",
                "firmware.bin",
                "home-assistant/README.md",
                "home-assistant/dashboard-native.yaml",
                "home-assistant/dashboard-animated.yaml",
                "home-assistant/view-native.yaml",
                "home-assistant/view-animated.yaml",
                "home-assistant/view-custom-card.yaml",
                "home-assistant/custom-card/gerlcraft-zehnder-card.js",
                "home-assistant/custom-card/INSTALLATION.md",
                "home-assistant/node-red/zehnder-bad-feuchte-boost.json",
            ):
                if required_name not in names:
                    errors.append(f"Fehlt im Release-ZIP: {required_name}")

    dashboard_archive = (
        SITE_DIR / "downloads" / "zehnder-comfoair-dashboard.zip"
    )

    if dashboard_archive.is_file():
        with zipfile.ZipFile(dashboard_archive, "r") as archive:
            bad_file = archive.testzip()

            if bad_file is not None:
                errors.append(f"Defektes Dashboard-ZIP: {bad_file}")

            names = set(archive.namelist())

            for required_name in (
                "README.md",
                "dashboard-native.yaml",
                "dashboard-animated.yaml",
                "view-native.yaml",
                "view-animated.yaml",
                "view-custom-card.yaml",
                "custom-card/gerlcraft-zehnder-card.js",
                "custom-card/INSTALLATION.md",
                "node-red/zehnder-bad-feuchte-boost.json",
            ):
                if required_name not in names:
                    errors.append(
                        f"Fehlt im Dashboard-ZIP: {required_name}"
                    )

    if errors:
        print("Release-Pruefung fehlgeschlagen:", file=sys.stderr)

        for error in errors:
            print(f"- {error}", file=sys.stderr)

        return 1

    print("Release-Pruefung erfolgreich")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
