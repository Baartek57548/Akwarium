#include "FakeU8g2.h"

#include "Arduino.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <random>

namespace {

std::atomic<unsigned long> g_millis{0UL};
std::mt19937 g_rng{0xA4C01A9Du};

const BitmapFont &font4x6() {
  static const BitmapFont font = BitmapFont::CreateU8g2_4x6();
  return font;
}

const BitmapFont &font5x8() {
  static const BitmapFont font = BitmapFont::CreateU8g2_5x8();
  return font;
}

const BitmapFont &font6x10() {
  static const BitmapFont font = BitmapFont::CreateU8g2_6x10();
  return font;
}

const BitmapFont &selectFont(const std::uint8_t *fontId) {
  if (fontId == u8g2_font_4x6_tr) {
    return font4x6();
  }

  if (fontId == u8g2_font_5x8_tr || fontId == u8g2_font_5x7_tr) {
    return font5x8();
  }

  if (fontId == u8g2_font_6x10_tr || fontId == u8g2_font_6x13_tr ||
      fontId == u8g2_font_profont10_tr || fontId == u8g2_font_profont11_tr ||
      fontId == u8g2_font_profont15_tr || fontId == u8g2_font_t0_17_tr ||
      fontId == u8g2_font_timR14_tr || fontId == u8g2_font_timR24_tr) {
    return font6x10();
  }

  return font6x10();
}

std::uint32_t decodeUtf8CodePoint(const unsigned char *&cursor) {
  const auto b0 = *cursor++;
  if (b0 < 0x80U) {
    return b0;
  }

  if ((b0 & 0xE0U) == 0xC0U) {
    const auto b1 = *cursor;
    if (b1 != 0U && (b1 & 0xC0U) == 0x80U) {
      cursor++;
      const std::uint32_t cp = static_cast<std::uint32_t>(((b0 & 0x1FU) << 6U) | (b1 & 0x3FU));
      if (cp >= 0x80U) {
        return cp;
      }
    }
    return static_cast<std::uint32_t>('?');
  }

  if ((b0 & 0xF0U) == 0xE0U) {
    const auto b1 = cursor[0];
    const auto b2 = cursor[1];
    if (b1 != 0U && b2 != 0U && (b1 & 0xC0U) == 0x80U && (b2 & 0xC0U) == 0x80U) {
      cursor += 2;
      const std::uint32_t cp = static_cast<std::uint32_t>(((b0 & 0x0FU) << 12U) |
                                                           ((b1 & 0x3FU) << 6U) |
                                                           (b2 & 0x3FU));
      if (cp >= 0x800U && !(cp >= 0xD800U && cp <= 0xDFFFU)) {
        return cp;
      }
    }
    return static_cast<std::uint32_t>('?');
  }

  if ((b0 & 0xF8U) == 0xF0U) {
    const auto b1 = cursor[0];
    const auto b2 = cursor[1];
    const auto b3 = cursor[2];
    if (b1 != 0U && b2 != 0U && b3 != 0U &&
        (b1 & 0xC0U) == 0x80U && (b2 & 0xC0U) == 0x80U && (b3 & 0xC0U) == 0x80U) {
      cursor += 3;
      const std::uint32_t cp = static_cast<std::uint32_t>(((b0 & 0x07U) << 18U) |
                                                           ((b1 & 0x3FU) << 12U) |
                                                           ((b2 & 0x3FU) << 6U) |
                                                           (b3 & 0x3FU));
      if (cp >= 0x10000U && cp <= 0x10FFFFU) {
        return cp;
      }
    }
    return static_cast<std::uint32_t>('?');
  }

  return static_cast<std::uint32_t>('?');
}

} // namespace

const std::uint8_t u8g2_font_5x7_tr[] = {0x03};
const std::uint8_t u8g2_font_6x13_tr[] = {0x05};
const std::uint8_t u8g2_font_profont10_tr[] = {0x06};
const std::uint8_t u8g2_font_profont11_tr[] = {0x07};
const std::uint8_t u8g2_font_profont15_tr[] = {0x08};
const std::uint8_t u8g2_font_t0_17_tr[] = {0x09};
const std::uint8_t u8g2_font_timR14_tr[] = {0x0A};
const std::uint8_t u8g2_font_timR24_tr[] = {0x0B};

unsigned long millis() { return g_millis.load(std::memory_order_relaxed); }

void delay(unsigned long ms) { fake_advance_millis(ms); }

long random(long maxExclusive) {
  if (maxExclusive <= 0) {
    return 0;
  }
  std::uniform_int_distribution<long> distribution(0, maxExclusive - 1);
  return distribution(g_rng);
}

long random(long minInclusive, long maxExclusive) {
  if (maxExclusive <= minInclusive) {
    return minInclusive;
  }
  std::uniform_int_distribution<long> distribution(minInclusive, maxExclusive - 1);
  return distribution(g_rng);
}

void fake_set_millis(unsigned long value) { g_millis.store(value, std::memory_order_relaxed); }

void fake_advance_millis(unsigned long deltaMs) { g_millis.fetch_add(deltaMs, std::memory_order_relaxed); }

U8G2::U8G2() : currentFont_(&font6x10()) { clearBuffer(); }

void U8G2::setFontMode(std::uint8_t mode) { fontMode_ = mode; }

void U8G2::setBitmapMode(std::uint8_t mode) { bitmapMode_ = mode; }

void U8G2::setDrawColor(std::uint8_t color) { drawColor_ = static_cast<std::uint8_t>(color == 0 ? 0 : 1); }

void U8G2::setFont(const std::uint8_t *font) { currentFont_ = &selectFont(font); }

void U8G2::clearBuffer() {
  std::memset(framebuffer_, 0, sizeof(framebuffer_));
  linearFrameBuffer_.fill(0);
}

void U8G2::sendBuffer() { publishedLinearFrameBuffer_ = linearFrameBuffer_; }

void U8G2::setPixelInternal(int x, int y) {
  if (x < 0 || x >= DisplayWidth || y < 0 || y >= DisplayHeight) {
    return;
  }

  const std::uint8_t pixel = drawColor_ == 0 ? 0 : 1;
  framebuffer_[x][y] = pixel;
  linearFrameBuffer_[static_cast<std::size_t>((x * DisplayHeight) + y)] = pixel;
}

void U8G2::drawPixel(int x, int y) { setPixelInternal(x, y); }

void U8G2::drawLine(int x0, int y0, int x1, int y1) {
  int dx = std::abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -std::abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int error = dx + dy;

  while (true) {
    setPixelInternal(x0, y0);
    if (x0 == x1 && y0 == y1) {
      break;
    }

    const int twiceError = 2 * error;
    if (twiceError >= dy) {
      error += dy;
      x0 += sx;
    }
    if (twiceError <= dx) {
      error += dx;
      y0 += sy;
    }
  }
}

void U8G2::drawVLine(int x, int y, int height) {
  for (int offset = 0; offset < height; offset++) {
    setPixelInternal(x, y + offset);
  }
}

void U8G2::drawBox(int x, int y, int width, int height) {
  if (width <= 0 || height <= 0) {
    return;
  }

  for (int py = y; py < y + height; py++) {
    for (int px = x; px < x + width; px++) {
      setPixelInternal(px, py);
    }
  }
}

void U8G2::drawFrame(int x, int y, int width, int height) {
  if (width <= 0 || height <= 0) {
    return;
  }

  drawLine(x, y, x + width - 1, y);
  drawLine(x, y + height - 1, x + width - 1, y + height - 1);
  drawLine(x, y, x, y + height - 1);
  drawLine(x + width - 1, y, x + width - 1, y + height - 1);
}

void U8G2::drawCircle(int x0, int y0, int radius, std::uint8_t /*options*/) {
  if (radius <= 0) {
    return;
  }

  int x = radius;
  int y = 0;
  int err = 1 - x;

  while (x >= y) {
    setPixelInternal(x0 + x, y0 + y);
    setPixelInternal(x0 + y, y0 + x);
    setPixelInternal(x0 - y, y0 + x);
    setPixelInternal(x0 - x, y0 + y);
    setPixelInternal(x0 - x, y0 - y);
    setPixelInternal(x0 - y, y0 - x);
    setPixelInternal(x0 + y, y0 - x);
    setPixelInternal(x0 + x, y0 - y);
    y++;
    if (err < 0) {
      err += 2 * y + 1;
    } else {
      x--;
      err += 2 * (y - x + 1);
    }
  }
}

void U8G2::drawDisc(int x0, int y0, int radius, std::uint8_t /*options*/) {
  if (radius <= 0) {
    return;
  }

  for (int y = -radius; y <= radius; y++) {
    const int xSpan = static_cast<int>(std::floor(std::sqrt((radius * radius) - (y * y))));
    for (int x = -xSpan; x <= xSpan; x++) {
      setPixelInternal(x0 + x, y0 + y);
    }
  }
}

void U8G2::drawEllipse(int x0, int y0, int rx, int ry, std::uint8_t /*options*/) {
  if (rx <= 0 || ry <= 0) {
    return;
  }

  constexpr double twoPi = 6.28318530717958647692;
  for (int degree = 0; degree < 360; degree++) {
    const double angle = (twoPi * static_cast<double>(degree)) / 360.0;
    const int x = x0 + static_cast<int>(std::round(static_cast<double>(rx) * std::cos(angle)));
    const int y = y0 + static_cast<int>(std::round(static_cast<double>(ry) * std::sin(angle)));
    setPixelInternal(x, y);
  }
}

void U8G2::drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2) {
  drawLine(x0, y0, x1, y1);
  drawLine(x1, y1, x2, y2);
  drawLine(x2, y2, x0, y0);
}

void U8G2::drawXBMP(int x, int y, int width, int height, const std::uint8_t *bitmap) {
  if (bitmap == nullptr || width <= 0 || height <= 0) {
    return;
  }

  const int bytesPerRow = (width + 7) / 8;
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      const int byteIndex = row * bytesPerRow + (col / 8);
      const std::uint8_t bits = bitmap[byteIndex];
      const bool isOn = (bits & static_cast<std::uint8_t>(1u << (col % 8))) != 0;
      if (isOn) {
        setPixelInternal(x + col, y + row);
      }
    }
  }
}

void U8G2::drawBitmap(int x, int y, int bytesPerRow, int height, const std::uint8_t *bitmap) {
  if (bitmap == nullptr || bytesPerRow <= 0 || height <= 0) {
    return;
  }

  const int width = bytesPerRow * 8;
  for (int row = 0; row < height; row++) {
    for (int col = 0; col < width; col++) {
      const int byteIndex = row * bytesPerRow + (col / 8);
      const std::uint8_t bits = bitmap[byteIndex];
      const bool isOn = (bits & static_cast<std::uint8_t>(0x80u >> (col % 8))) != 0;
      if (isOn) {
        setPixelInternal(x + col, y + row);
      }
    }
  }
}

std::uint8_t U8G2::drawGlyph(int x, int y, std::uint16_t encoding) {
  if (currentFont_ == nullptr) {
    currentFont_ = &font6x10();
  }

  const auto &glyph = currentFont_->GetGlyph(encoding);
  const int glyphWidth = std::max(0, glyph.width);
  const int glyphHeight = std::max(0, glyph.height);
  const int glyphTop = y - glyphHeight - glyph.yOffset;
  const int glyphLeft = x + glyph.xOffset;
  const int drawWidth = std::min(glyphWidth, 16);

  for (int row = 0; row < glyphHeight && row < static_cast<int>(glyph.rows.size()); row++) {
    const std::uint16_t bits = glyph.rows[static_cast<std::size_t>(row)];
    for (int col = 0; col < drawWidth; col++) {
      const bool on = (bits & static_cast<std::uint16_t>(1u << (drawWidth - 1 - col))) != 0;
      if (on) {
        setPixelInternal(glyphLeft + col, glyphTop + row);
      }
    }
  }

  const int advance =
      glyph.deltaX > 0 ? glyph.deltaX : (glyphWidth + std::max(0, currentFont_->Spacing()));
  return static_cast<std::uint8_t>(std::max(0, advance));
}

std::uint8_t U8G2::drawStr(int x, int y, const char *text) {
  if (text == nullptr || text[0] == '\0') {
    return 0;
  }

  const int startX = x;
  int cursorX = x;

  const auto *cursor = reinterpret_cast<const unsigned char *>(text);
  while (*cursor != '\0') {
    const std::uint32_t codePoint = decodeUtf8CodePoint(cursor);
    const std::uint16_t encoding =
        codePoint <= 0xFFFFU ? static_cast<std::uint16_t>(codePoint) : static_cast<std::uint16_t>('?');
    cursorX += drawGlyph(cursorX, y, encoding);
  }

  return static_cast<std::uint8_t>(std::max(0, cursorX - startX));
}

const std::uint8_t *U8G2::getLinearFrameBuffer() const { return linearFrameBuffer_.data(); }

const std::uint8_t *U8G2::getPublishedLinearFrameBuffer() const {
  return publishedLinearFrameBuffer_.data();
}

const std::uint8_t (&U8G2::getRawFrameBuffer() const)[DisplayWidth][DisplayHeight] {
  return framebuffer_;
}
