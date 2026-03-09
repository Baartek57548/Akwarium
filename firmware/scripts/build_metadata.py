# Canonical PlatformIO metadata script used by firmware builds.
# Keep this file. Repo-root PlatformIO runs use `scripts/build_metadata.py`,
# which delegates to this implementation.
Import("env")

import json
from pathlib import Path
import subprocess

project_dir = Path(env["PROJECT_DIR"]).resolve()
REPO_ROOT = project_dir if (project_dir / ".git").exists() else project_dir.parent


def run_git(args, fallback):
    try:
        output = subprocess.check_output(
            ["git", *args],
            cwd=REPO_ROOT,
            stderr=subprocess.DEVNULL,
        )
        return output.decode("utf-8").strip() or fallback
    except Exception:
        return fallback


firmware_version = run_git(["describe", "--tags", "--always", "--dirty"], "dev")
build_ref = run_git(["rev-parse", "--short", "HEAD"], "unknown")

env.Append(
    CPPDEFINES=[
        ("AQUARIUM_FIRMWARE_VERSION", '\\"%s\\"' % firmware_version),
        ("AQUARIUM_FIRMWARE_BUILD_REF", '\\"%s\\"' % build_ref),
    ]
)

print(
    "[build_metadata] version=%s ref=%s"
    % (firmware_version, build_ref)
)
