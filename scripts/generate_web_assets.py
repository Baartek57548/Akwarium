# Compatibility shim for PlatformIO runs started from the repository root.
# DO NOT DELETE: `platformio run` from repo root resolves
# `scripts/generate_web_assets.py` here, while firmware-local runs use
# `firmware/scripts/generate_web_assets.py`.

Import("env")

from pathlib import Path


canonical_script = (
    Path(env["PROJECT_DIR"]).resolve() / "firmware" / "scripts" / "generate_web_assets.py"
)

exec(
    compile(canonical_script.read_text(encoding="utf-8"), str(canonical_script), "exec"),
    globals(),
    globals(),
)
