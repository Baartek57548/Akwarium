#ifndef FAKE_U8G2_H
#define FAKE_U8G2_H

#include "BitmapFont.h"

#include <array>
#include <cstdint>

constexpr std::uint8_t U8G2_DRAW_ALL = 0xFF;

class U8G2 {
public:
  static constexpr int DisplayWidth = 128;
  static constexpr int DisplayHeight = 32;

  U8G2();
  virtual ~U8G2() = default;

  void setFontMode(std::uint8_t mode);
  void setBitmapMode(std::uint8_t mode);
  void setDrawColor(std::uint8_t color);
  void setFont(const std::uint8_t *font);

  void clearBuffer();
  void sendBuffer();

  void drawPixel(int x, int y);
  void drawLine(int x0, int y0, int x1, int y1);
  void drawVLine(int x, int y, int height);
  void drawBox(int x, int y, int width, int height);
  void drawFrame(int x, int y, int width, int height);
  void drawCircle(int x0, int y0, int radius, std::uint8_t options = U8G2_DRAW_ALL);
  void drawDisc(int x0, int y0, int radius, std::uint8_t options = U8G2_DRAW_ALL);
  void drawEllipse(int x0, int y0, int rx, int ry, std::uint8_t options = U8G2_DRAW_ALL);
  void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2);

  void drawXBMP(int x, int y, int width, int height, const std::uint8_t *bitmap);
  void drawBitmap(int x, int y, int bytesPerRow, int height, const std::uint8_t *bitmap);
  std::uint8_t drawGlyph(int x, int y, std::uint16_t encoding);
  std::uint8_t drawStr(int x, int y, const char *text);

  const std::uint8_t *getLinearFrameBuffer() const;
  const std::uint8_t *getPublishedLinearFrameBuffer() const;
  const std::uint8_t (&getRawFrameBuffer() const)[DisplayWidth][DisplayHeight];

protected:
  void setPixelInternal(int x, int y);

private:
  std::uint8_t drawColor_ = 1;
  std::uint8_t fontMode_ = 1;
  std::uint8_t bitmapMode_ = 1;
  const BitmapFont *currentFont_ = nullptr;

  std::uint8_t framebuffer_[DisplayWidth][DisplayHeight]{};
  std::array<std::uint8_t, DisplayWidth * DisplayHeight> linearFrameBuffer_{};
  std::array<std::uint8_t, DisplayWidth * DisplayHeight> publishedLinearFrameBuffer_{};
};

class FakeU8g2 final : public U8G2 {
public:
  FakeU8g2() = default;
  ~FakeU8g2() override = default;
};

extern const std::uint8_t u8g2_font_4x6_tr[];
extern const std::uint8_t u8g2_font_5x8_tr[];
extern const std::uint8_t u8g2_font_5x7_tr[];
extern const std::uint8_t u8g2_font_6x10_tr[];
extern const std::uint8_t u8g2_font_6x13_tr[];
extern const std::uint8_t u8g2_font_profont10_tr[];
extern const std::uint8_t u8g2_font_profont11_tr[];
extern const std::uint8_t u8g2_font_profont15_tr[];
extern const std::uint8_t u8g2_font_t0_17_tr[];
extern const std::uint8_t u8g2_font_timR14_tr[];
extern const std::uint8_t u8g2_font_timR24_tr[];

#endif
