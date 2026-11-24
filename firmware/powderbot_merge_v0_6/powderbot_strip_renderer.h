#pragma once

#include <Adafruit_NeoPixel.h>

#include "powderbot_types.h"

class PowderBotStripRenderer {
public:
  PowderBotStripRenderer(Adafruit_NeoPixel &strip, uint16_t numPixels, const PbTimingConfig &timing);

  void begin(uint8_t brightness);
  void reset();
  void render(const PbRenderState &state, uint32_t now);

private:
  uint32_t colorOff() const;
  uint32_t colorRed(uint8_t level) const;
  uint32_t colorGreen(uint8_t level) const;
  uint32_t colorBlue(uint8_t level) const;
  uint32_t scaleColor(uint32_t color, float scale) const;
  uint32_t verdictColor(PbVerdict verdict) const;

  void clearStripNoShow();
  void showStrip();
  void fillAllNoShow(uint32_t color);
  void fillSymmetricGradientNoShow(int radius, uint32_t color);
  void drawScannerColorNoShow(int pos, uint8_t tail, uint32_t color, uint8_t maxLevel);
  void setPixelColorClamped(int index, uint32_t color);
  void drawScannerNoShow(int pos, uint8_t tail, uint8_t maxLevel);
  int computeCenteredScannerPos(uint32_t elapsedMs, uint16_t frameMs) const;
  void resetScannerMotionToCenter();
  void stepScannerMotion();

  void updateStartupVisual(uint32_t now, const PbRenderState &state);
  void updateBleWakeVisual(uint32_t now, const PbRenderState &state);
  void updateDisconnectVisual(uint32_t now, const PbRenderState &state);
  void updateStopAnimVisual(uint32_t now, const PbRenderState &state);
  void updateIdleEmberVisual(uint32_t now, const PbRenderState &state);
  void updateHeartbeatVisual(uint32_t now, const PbRenderState &state);
  void updateTargetAckVisual(uint32_t now, const PbRenderState &state);
  void updateScannerVisual(uint32_t now, const PbRenderState &state);
  void updatePassAnimVisual(uint32_t now, const PbRenderState &state);
  void updateFailAnimVisual(uint32_t now, const PbRenderState &state);
  void updateSolidHoldVisual(uint32_t color, uint32_t now);
  void updateResetWipeVisual(uint32_t now, const PbRenderState &state);

  Adafruit_NeoPixel &strip_;
  const uint16_t numPixels_;
  const PbTimingConfig &timing_;

  PbVisualMode lastMode_ = PbVisualMode::Off;
  uint32_t lastFrameMs_ = 0;
  int scannerPos_ = 0;
  int scannerDir_ = 1;
  bool scannerPrimed_ = false;
};
