Import("env")

from pathlib import Path
import gzip
import io


project_dir = Path(env["PROJECT_DIR"]).resolve()
FIRMWARE_DIR = (
    project_dir
    if (project_dir / "web").exists() and (project_dir / "src").exists()
    else project_dir / "firmware"
)
WEB_DIR = FIRMWARE_DIR / "web"
OUTPUT_FILE = FIRMWARE_DIR / "src" / "WebAssets.h"

ASSETS = [
    ("index.html", "text/html; charset=utf-8", "web_index_html_gz"),
    ("style.css", "text/css; charset=utf-8", "web_style_css_gz"),
    ("script.js", "application/javascript; charset=utf-8", "web_script_js_gz"),
]


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


def build_header():
    lines = [
        "#ifndef WEB_ASSETS_H",
        "#define WEB_ASSETS_H",
        "",
        "#include <Arduino.h>",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "struct WebAssetDescriptor {",
        "  const char *contentType;",
        "  const uint8_t *data;",
        "  size_t size;",
        "};",
        "",
    ]

    descriptor_lines = []

    for relative_path, content_type, symbol in ASSETS:
        source_path = WEB_DIR / relative_path
        raw_bytes = source_path.read_bytes()
        compressed = gzip_bytes(raw_bytes)

        lines.extend(
            [
                f"static const uint8_t {symbol}[] PROGMEM = {{",
                format_bytes(compressed),
                "};",
                f"static const WebAssetDescriptor {symbol}_asset = {{",
                f'  "{content_type}",',
                f"  {symbol},",
                f"  sizeof({symbol})",
                "};",
                "",
            ]
        )

        descriptor_lines.append(f"static constexpr const char *{symbol}_source = \"/{relative_path}\";")

    lines.extend(descriptor_lines)
    lines.extend(["", "#endif // WEB_ASSETS_H", ""])
    return "\n".join(lines)


WEB_DIR.mkdir(parents=True, exist_ok=True)
OUTPUT_FILE.write_text(build_header(), encoding="utf-8", newline="\n")
print(
    f"[generate_web_assets] wrote {OUTPUT_FILE.relative_to(FIRMWARE_DIR)} "
    f"from {WEB_DIR.relative_to(FIRMWARE_DIR)}"
)
