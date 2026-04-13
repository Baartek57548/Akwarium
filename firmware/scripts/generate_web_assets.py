Import("env")

from pathlib import Path
import gzip
import io
import re


project_dir = Path(env["PROJECT_DIR"]).resolve()
FIRMWARE_DIR = (
    project_dir
    if (project_dir / "web").exists() and (project_dir / "src").exists()
    else project_dir / "firmware"
)
WEB_DIR = FIRMWARE_DIR / "web"
OUTPUT_FILE = FIRMWARE_DIR / "src" / "WebAssets.h"

CONTENT_TYPES = {
    ".html": "text/html; charset=utf-8",
    ".css": "text/css; charset=utf-8",
    ".js": "application/javascript; charset=utf-8",
    ".json": "application/json; charset=utf-8",
    ".svg": "image/svg+xml",
    ".txt": "text/plain; charset=utf-8",
}


def gzip_bytes(raw_bytes):
    buffer = io.BytesIO()
    with gzip.GzipFile(fileobj=buffer, mode="wb", compresslevel=9, mtime=0) as gz:
        gz.write(raw_bytes)
    return buffer.getvalue()


def format_bytes(data):
    chunks = []
    row = []
    for index, value in enumerate(data, start=1):
        row.append(f"0x{value:02x}")
        if index % 12 == 0:
            chunks.append("  " + ", ".join(row))
            row = []
    if row:
        chunks.append("  " + ", ".join(row))
    return ",\n".join(chunks)


def asset_symbol_for(relative_path):
    normalized = relative_path.replace("\\", "/")
    symbol = re.sub(r"[^a-zA-Z0-9]+", "_", normalized).strip("_").lower()
    return f"web_{symbol}_gz"


def detect_content_type(relative_path):
    suffix = Path(relative_path).suffix.lower()
    return CONTENT_TYPES.get(suffix, "application/octet-stream")


def collect_assets():
    assets = []
    for source_path in sorted(path for path in WEB_DIR.rglob("*") if path.is_file()):
        relative_path = source_path.relative_to(WEB_DIR).as_posix()
        assets.append(
            {
                "relative_path": relative_path,
                "content_type": detect_content_type(relative_path),
                "symbol": asset_symbol_for(relative_path),
            }
        )
    return assets


def build_header():
    assets = collect_assets()
    lines = [
        "#ifndef WEB_ASSETS_H",
        "#define WEB_ASSETS_H",
        "",
        "#include <Arduino.h>",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "struct WebAssetDescriptor {",
        "  const char *path;",
        "  const char *contentType;",
        "  const uint8_t *data;",
        "  size_t size;",
        "};",
        "",
    ]

    descriptor_names = []

    for asset in assets:
        relative_path = asset["relative_path"]
        content_type = asset["content_type"]
        symbol = asset["symbol"]
        source_path = WEB_DIR / relative_path
        raw_bytes = source_path.read_bytes()
        compressed = gzip_bytes(raw_bytes)

        lines.extend(
            [
                f"static const uint8_t {symbol}[] PROGMEM = {{",
                format_bytes(compressed),
                "};",
                f"static const WebAssetDescriptor {symbol}_asset = {{",
                f'  "/{relative_path}",',
                f'  "{content_type}",',
                f"  {symbol},",
                f"  sizeof({symbol})",
                "};",
                "",
            ]
        )
        descriptor_names.append(f"  &{symbol}_asset,")

    lines.append("static const WebAssetDescriptor *const web_asset_table[] = {")
    lines.extend(descriptor_names)
    lines.extend(
        [
            "};",
            "",
            "static constexpr size_t web_asset_table_count =",
            "    sizeof(web_asset_table) / sizeof(web_asset_table[0]);",
            "",
            "#endif // WEB_ASSETS_H",
            "",
        ]
    )
    return "\n".join(lines)


WEB_DIR.mkdir(parents=True, exist_ok=True)
OUTPUT_FILE.write_text(build_header(), encoding="utf-8", newline="\n")
print(
    f"[generate_web_assets] wrote {OUTPUT_FILE.relative_to(FIRMWARE_DIR)} "
    f"from {WEB_DIR.relative_to(FIRMWARE_DIR)}"
)
