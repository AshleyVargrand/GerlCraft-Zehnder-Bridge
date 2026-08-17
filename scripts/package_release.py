Import("env")

import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import zipfile


PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))
BUILD_DIR = Path(env.subst("$BUILD_DIR"))
ENVIRONMENT = env.subst("$PIOENV")


def sha256(path):
    digest = hashlib.sha256()

    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)

    return digest.hexdigest()


def read_version():
    app_config = (
        PROJECT_DIR / "include" / "core" / "app_config.h"
    ).read_text(encoding="utf-8")

    match = re.search(
        r'FIRMWARE_VERSION\[\]\s*=\s*"([^"]+)"',
        app_config,
    )

    if match is None:
        raise RuntimeError("Firmwareversion konnte nicht gelesen werden")

    return match.group(1)


def git_value(*arguments):
    try:
        return subprocess.check_output(
            ["git", "-C", str(PROJECT_DIR), *arguments],
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def package_version(package_dir):
    metadata_files = [
        package_dir / "package.json",
        package_dir / ".piopm",
    ]

    for metadata_file in metadata_files:
        if not metadata_file.exists():
            continue

        try:
            metadata = json.loads(
                metadata_file.read_text(encoding="utf-8")
            )
        except (OSError, json.JSONDecodeError):
            continue

        version = metadata.get("version")

        if version:
            return str(version)

    return "unknown"


def copy_legal_files(destination):
    destination.mkdir(parents=True, exist_ok=True)

    for filename in (
        "LICENSE",
        "CREDITS.md",
        "THIRD_PARTY_NOTICES.md",
    ):
        shutil.copy2(PROJECT_DIR / filename, destination / filename)

    shutil.copytree(
        PROJECT_DIR / "LICENSES",
        destination / "LICENSES",
        dirs_exist_ok=True,
    )


def clear_directory(directory):
    directory.mkdir(parents=True, exist_ok=True)

    for child in directory.iterdir():
        if child.is_dir():
            shutil.rmtree(child)
        else:
            child.unlink()


def deterministic_zip(source_dir, destination):
    with zipfile.ZipFile(
        destination,
        "w",
        compression=zipfile.ZIP_DEFLATED,
        compresslevel=9,
    ) as archive:
        for source_path in sorted(
            path for path in source_dir.rglob("*") if path.is_file()
        ):
            relative_path = source_path.relative_to(source_dir).as_posix()
            zip_info = zipfile.ZipInfo(relative_path)
            zip_info.date_time = (1980, 1, 1, 0, 0, 0)
            zip_info.compress_type = zipfile.ZIP_DEFLATED
            zip_info.external_attr = 0o644 << 16
            archive.writestr(zip_info, source_path.read_bytes())


def package_dashboard(site_dir, release_dir):
    dashboard_source = PROJECT_DIR / "home-assistant"
    required_files = (
        dashboard_source / "README.md",
        dashboard_source / "dashboard-native.yaml",
        dashboard_source / "dashboard-animated.yaml",
        dashboard_source / "view-native.yaml",
        dashboard_source / "view-animated.yaml",
        dashboard_source / "view-animated-v2.yaml",
        dashboard_source / "view-custom-card.yaml",
        dashboard_source / "custom-card" / "gerlcraft-zehnder-card.js",
        dashboard_source / "custom-card" / "INSTALLATION.md",
        dashboard_source / "node-red" / "zehnder-bad-feuchte-boost.json",
    )

    for required_file in required_files:
        if not required_file.is_file():
            raise RuntimeError(
                f"Home-Assistant-Datei fehlt: {required_file}"
            )

    release_dashboard_dir = release_dir / "home-assistant"
    shutil.copytree(
        dashboard_source,
        release_dashboard_dir,
        dirs_exist_ok=True,
    )

    download_dir = site_dir / "downloads"
    download_dir.mkdir(parents=True, exist_ok=True)
    archive_path = download_dir / "zehnder-comfoair-dashboard.zip"
    checksum_path = download_dir / "zehnder-comfoair-dashboard.sha256"

    deterministic_zip(dashboard_source, archive_path)
    checksum_path.write_text(
        f"{sha256(archive_path)}  {archive_path.name}\n",
        encoding="ascii",
    )

    return archive_path


def build_release(source, target, env):
    version = read_version()
    artifact_name = f"zehnder-comfoair-bridge-v{version}"

    platform = env.PioPlatform()
    framework_dir = Path(
        platform.get_package_dir("framework-arduinoespressif32")
    )
    framework_libs_dir = Path(
        platform.get_package_dir("framework-arduinoespressif32-libs")
    )
    esptool_dir = Path(platform.get_package_dir("tool-esptoolpy"))

    bootloader = BUILD_DIR / "bootloader.bin"
    partitions = BUILD_DIR / "partitions.bin"
    boot_app0 = framework_dir / "tools" / "partitions" / "boot_app0.bin"
    application = BUILD_DIR / "firmware.bin"
    esptool = esptool_dir / "esptool.py"

    required_files = (
        bootloader,
        partitions,
        boot_app0,
        application,
        esptool,
    )

    for required_file in required_files:
        if not required_file.exists():
            raise RuntimeError(f"Release-Datei fehlt: {required_file}")

    dist_dir = PROJECT_DIR / "dist"
    site_dir = dist_dir / "browser-installer"
    firmware_dir = site_dir / "firmware"
    release_dir = dist_dir / "release-package"

    clear_directory(site_dir)
    clear_directory(release_dir)
    firmware_dir.mkdir(parents=True, exist_ok=True)

    for old_archive in dist_dir.glob(
        "zehnder-comfoair-bridge-v*.zip"
    ):
        old_archive.unlink()

    for old_checksum in dist_dir.glob(
        "zehnder-comfoair-bridge-v*.sha256"
    ):
        old_checksum.unlink()

    factory_filename = f"{artifact_name}.factory.bin"
    factory_firmware = firmware_dir / factory_filename

    python_executable = env.subst("$PYTHONEXE") or sys.executable

    subprocess.run(
        [
            python_executable,
            str(esptool),
            "--chip",
            "esp32s3",
            "merge_bin",
            "-o",
            str(factory_firmware),
            "--flash_mode",
            "dio",
            "--flash_freq",
            "80m",
            "--flash_size",
            "8MB",
            "0x0",
            str(bootloader),
            "0x8000",
            str(partitions),
            "0xe000",
            str(boot_app0),
            "0x10000",
            str(application),
        ],
        check=True,
    )

    manifest = {
        "name": "Zehnder ComfoAir Bridge",
        "version": version,
        "new_install_prompt_erase": True,
        "new_install_improv_wait_time": 0,
        "builds": [
            {
                "chipFamily": "ESP32-S3",
                "improv": False,
                "parts": [
                    {
                        "path": f"firmware/{factory_filename}",
                        "offset": 0,
                    }
                ],
            }
        ],
    }

    manifest_path = site_dir / "manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2, ensure_ascii=True) + "\n",
        encoding="utf-8",
    )

    shutil.copy2(
        PROJECT_DIR / "web-installer" / "index.html",
        site_dir / "index.html",
    )

    dashboard_archive = package_dashboard(site_dir, release_dir)

    commit = git_value("rev-parse", "HEAD")
    commit_time = git_value("show", "-s", "--format=%cI", "HEAD")
    repository = os.environ.get(
        "GITHUB_SERVER_URL",
        "https://github.com",
    )
    repository_name = os.environ.get(
        "GITHUB_REPOSITORY",
        "AshleyVargrand/GerlCraft-Zehnder-Bridge",
    )
    source_url = f"{repository}/{repository_name}/tree/{commit}"

    build_info = {
        "firmware": "Zehnder ComfoAir Bridge",
        "version": version,
        "source_commit": commit,
        "source_timestamp": commit_time,
        "source_url": source_url,
        "environment": ENVIRONMENT,
        "board": env.subst("$BOARD"),
        "platform": (
            "pioarduino/platform-espressif32@53.03.13"
        ),
        "arduino_esp32": package_version(framework_dir),
        "arduino_esp32_libs": package_version(framework_libs_dir),
        "esptool": package_version(esptool_dir),
        "esp32mqttclient_commit": (
            "da1cc95608830b46520fe7e74eafce7de0158fa7"
        ),
        "flash": {
            "chip": "ESP32-S3",
            "size": "8MB",
            "mode": "dio",
            "frequency": "80m",
            "factory_offset": 0,
        },
        "artifacts": {
            factory_filename: sha256(factory_firmware),
            "firmware.bin": sha256(application),
            dashboard_archive.name: sha256(dashboard_archive),
        },
    }

    build_info_path = site_dir / "build-info.json"
    build_info_path.write_text(
        json.dumps(build_info, indent=2, ensure_ascii=True) + "\n",
        encoding="utf-8",
    )

    legal_dir = site_dir / "legal"
    copy_legal_files(legal_dir)

    shutil.copy2(factory_firmware, release_dir / factory_filename)
    shutil.copy2(application, release_dir / "firmware.bin")
    shutil.copy2(manifest_path, release_dir / "manifest.json")
    shutil.copy2(build_info_path, release_dir / "build-info.json")
    copy_legal_files(release_dir)

    source_text = (
        f"Zugehoeriger Quellcode fuer Firmware v{version}:\n"
        f"{source_url}\n\n"
        "Lizenz: GNU GPL Version 3\n"
        "Siehe LICENSE und THIRD_PARTY_NOTICES.md.\n"
    )
    (release_dir / "SOURCE.txt").write_text(
        source_text,
        encoding="utf-8",
    )

    checksum_lines = [
        f"{sha256(release_dir / factory_filename)}  {factory_filename}",
        f"{sha256(release_dir / 'firmware.bin')}  firmware.bin",
        f"{sha256(release_dir / 'manifest.json')}  manifest.json",
        f"{sha256(release_dir / 'build-info.json')}  build-info.json",
        (
            f"{sha256(release_dir / 'home-assistant' / 'README.md')}  "
            "home-assistant/README.md"
        ),
        (
            f"{sha256(release_dir / 'home-assistant' / 'dashboard-native.yaml')}  "
            "home-assistant/dashboard-native.yaml"
        ),
        (
            f"{sha256(release_dir / 'home-assistant' / 'dashboard-animated.yaml')}  "
            "home-assistant/dashboard-animated.yaml"
        ),
    ]
    (release_dir / "SHA256SUMS.txt").write_text(
        "\n".join(checksum_lines) + "\n",
        encoding="ascii",
    )

    archive_path = dist_dir / f"{artifact_name}.zip"
    deterministic_zip(release_dir, archive_path)
    (dist_dir / f"{artifact_name}.sha256").write_text(
        f"{sha256(archive_path)}  {archive_path.name}\n",
        encoding="ascii",
    )

    print(f"Release-Paket erstellt: {archive_path}")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", build_release)
