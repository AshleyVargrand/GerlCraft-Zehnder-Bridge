from pathlib import Path
import re
import subprocess
import sys


PROJECT_DIR = Path(__file__).resolve().parents[1]

FORBIDDEN_TRACKED_PATHS = {
    ".pio",
    "dist",
    "include/secrets.h",
    "include/user_config.h",
    "src/secrets.h",
    "secrets.ini",
}

SECRET_PATTERNS = {
    "private key": re.compile(
        rb"-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----"
    ),
    "GitHub token": re.compile(rb"(?:ghp_|github_pat_)[A-Za-z0-9_]{20,}"),
    "AWS access key": re.compile(rb"AKIA[0-9A-Z]{16}"),
    "Google API key": re.compile(rb"AIza[0-9A-Za-z_-]{30,}"),
}


def tracked_files():
    result = subprocess.run(
        ["git", "-C", str(PROJECT_DIR), "ls-files", "-z"],
        check=True,
        capture_output=True,
    )

    return [
        Path(item.decode("utf-8"))
        for item in result.stdout.split(b"\0")
        if item
    ]


def main():
    errors = []

    for relative_path in tracked_files():
        normalized = relative_path.as_posix()
        top_level = normalized.split("/", 1)[0]

        if (
            normalized in FORBIDDEN_TRACKED_PATHS
            or top_level in FORBIDDEN_TRACKED_PATHS
        ):
            errors.append(f"Nicht versionieren: {normalized}")
            continue

        absolute_path = PROJECT_DIR / relative_path

        if not absolute_path.is_file():
            continue

        data = absolute_path.read_bytes()

        for label, pattern in SECRET_PATTERNS.items():
            if pattern.search(data):
                errors.append(f"Moegliches Geheimnis ({label}): {normalized}")

    if errors:
        print("Repository-Pruefung fehlgeschlagen:", file=sys.stderr)

        for error in errors:
            print(f"- {error}", file=sys.stderr)

        return 1

    print("Repository-Pruefung erfolgreich")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
