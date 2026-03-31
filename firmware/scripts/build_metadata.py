# Canonical PlatformIO metadata script used by firmware builds.
# Keep this file. Repo-root PlatformIO runs use `scripts/build_metadata.py`,
# which delegates to this implementation.
Import("env")

from pathlib import Path
import re
import subprocess

project_dir = Path(env["PROJECT_DIR"]).resolve()
REPO_ROOT = project_dir if (project_dir / ".git").exists() else project_dir.parent
VERSION_FILE = REPO_ROOT / "firmware" / "VERSION"
SEMVER_TAG_RE = re.compile(r"^v?(\d+)\.(\d+)\.(\d+)$")


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


def read_base_version():
    try:
        raw = VERSION_FILE.read_text(encoding="utf-8").strip()
    except Exception:
        return "0.0.0"

    match = SEMVER_TAG_RE.match(raw)
    if not match:
        return "0.0.0"
    return "%d.%d.%d" % tuple(int(part) for part in match.groups())


def parse_semver_tag(tag):
    match = SEMVER_TAG_RE.match(tag.strip())
    if not match:
        return None
    return tuple(int(part) for part in match.groups())


def get_highest_semver_tag():
    raw_tags = run_git(["tag", "--list"], "")
    best_tag = ""
    best_semver = None
    for raw_tag in raw_tags.splitlines():
        parsed = parse_semver_tag(raw_tag)
        if parsed is None:
            continue
        if best_semver is None or parsed > best_semver:
            best_semver = parsed
            best_tag = raw_tag.strip()
    return best_tag


def get_commits_since_tag(tag_name):
    if not tag_name:
        total = run_git(["rev-list", "--count", "HEAD"], "0")
        return int(total) if total.isdigit() else 0

    count = run_git(["rev-list", "--count", "%s..HEAD" % tag_name], "0")
    return int(count) if count.isdigit() else 0


base_version = read_base_version()
highest_tag = get_highest_semver_tag()
commits_since_tag = get_commits_since_tag(highest_tag)
firmware_version = (
    base_version if commits_since_tag <= 0 else "%s+%d" % (base_version, commits_since_tag)
)

build_ref = run_git(["rev-parse", "--short", "HEAD"], "unknown")
is_dirty = bool(run_git(["status", "--porcelain"], ""))
if is_dirty:
    build_ref = "%s-dirty" % build_ref

env.Append(
    CPPDEFINES=[
        ("AQUARIUM_FIRMWARE_BASE_VERSION", '\\"%s\\"' % base_version),
        ("AQUARIUM_FIRMWARE_VERSION", '\\"%s\\"' % firmware_version),
        ("AQUARIUM_FIRMWARE_BUILD_REF", '\\"%s\\"' % build_ref),
    ]
)

print(
    "[build_metadata] base=%s version=%s ref=%s tag=%s commits_since_tag=%d"
    % (base_version, firmware_version, build_ref, highest_tag or "-", commits_since_tag)
)
