#include "powderbot_strip_renderer.h"

#include <Arduino.h>
#include <math.h>

PowderBotStripRenderer::PowderBotStripRenderer(Adafruit_NeoPixel &strip, uint16_t numPixels, const PbTimingConfig &timing)
  : strip_(strip), numPixels_(numPixels), timing_(timing) {
  resetScannerMotionToCenter();
}

void PowderBotStripRenderer::begin(uint8_t brightness) {
  strip_.begin();
  strip_.setBrightness(brightness);
  strip_.show();
  clearStripNoShow();
  showStrip();
}

void PowderBotStripRenderer::reset() {
  lastMode_ = PbVisualMode::Off;
  lastFrameMs_ = 0;
  resetScannerMotionToCenter();
  clearStripNoShow();
  showStrip();
}

uint32_t PowderBotStripRenderer::colorOff() const { return strip_.Color(0, 0, 0); }
uint32_t PowderBotStripRenderer::colorRed(uint8_t level) const { return strip_.Color(level, 0, 0); }
uint32_t PowderBotStripRenderer::colorGreen(uint8_t level) const { return strip_.Color(0, level, 0); }
uint32_t PowderBotStripRenderer::colorBlue(uint8_t level) const { return strip_.Color(0, 0, level); }

uint32_t PowderBotStripRenderer::scaleColor(uint32_t color, float scale) const {
  scale = constrain(scale, 0.0f, 1.0f);
  const uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * scale);
  const uint8_t g = (uint8_t)(((color >> 8) & 0xFF) * scale);
  const uint8_t b = (uint8_t)((color & 0xFF) * scale);
  return strip_.Color(r, g, b);
}

uint32_t PowderBotStripRenderer::verdictColor(PbVerdict verdict) const {
  return (verdict == PbVerdict::Pass) ? colorGreen(255) : colorRed(255);
}

void PowderBotStripRenderer::clearStripNoShow() {
  strip_.clear();
}

void PowderBotStripRenderer::showStrip() {
  strip_.show();
}

void PowderBotStripRenderer::fillAllNoShow(uint32_t color) {
  for (uint16_t i = 0; i < numPixels_; ++i) {
    strip_.setPixelColor(i, color);
  }
}

void PowderBotStripRenderer::fillSymmetricGradientNoShow(int radius, uint32_t color) {
  const int leftCenter = (numPixels_ / 2) - 1;
  const int rightCenter = (numPixels_ / 2);

  strip_.clear();
  for (int d = 0; d <= radius; ++d) {
    const float scale = 0.35f + 0.65f * ((float)(radius - d + 1) / (float)(radius + 1));
    const uint32_t c = scaleColor(color, scale);
    const int left = leftCenter - d;
    const int right = rightCenter + d;
    if (left >= 0) strip_.setPixelColor(left, c);
    if (right < (int)numPixels_) strip_.setPixelColor(right, c);
  }
}

void PowderBotStripRenderer::drawScannerColorNoShow(int pos, uint8_t tail, uint32_t color, uint8_t maxLevel) {
  strip_.clear();
  for (int i = 0; i < (int)numPixels_; ++i) {
    const int dist = abs(i - pos);
    if (dist <= tail) {
      float scale = 1.0f - ((float)dist / (float)(tail + 1));
      scale *= scale;
      strip_.setPixelColor(i, scaleColor(color, ((float)maxLevel / 255.0f) * scale));
    }
  }
}

void PowderBotStripRenderer::setPixelColorClamped(int index, uint32_t color) {
  if (index < 0 || index >= (int)numPixels_) return;
  strip_.setPixelColor(index, color);
}

void PowderBotStripRenderer::drawScannerNoShow(int pos, uint8_t, uint8_t maxLevel) {
  strip_.clear();

  const uint8_t headLevel = maxLevel;
  const uint8_t innerLevel = (uint8_t)((maxLevel * 90U) / 255U);
  const uint8_t outerLevel = (uint8_t)((maxLevel * 25U) / 255U);

  setPixelColorClamped(pos - 2, colorRed(outerLevel));
  setPixelColorClamped(pos - 1, colorRed(innerLevel));
  setPixelColorClamped(pos, colorRed(headLevel));
  setPixelColorClamped(pos + 1, colorRed(innerLevel));
  setPixelColorClamped(pos + 2, colorRed(outerLevel));
}

int PowderBotStripRenderer::computeCenteredScannerPos(uint32_t elapsedMs, uint16_t frameMs) const {
  const int centerPos = (numPixels_ - 1) / 2;
  const int rightEdge = numPixels_ - 1;
  const int leftEdge = 0;

  const int legCenterToRight = rightEdge - centerPos;
  const int legRightToLeft = rightEdge - leftEdge;
  const int legLeftToCenter = centerPos - leftEdge;
  const int totalTravel = legCenterToRight + legRightToLeft + legLeftToCenter;
  const int step = (int)((elapsedMs / frameMs) % totalTravel);

  if (step <= legCenterToRight) {
    return centerPos + step;
  }

  int tmp = step - legCenterToRight;
  if (tmp <= legRightToLeft) {
    return rightEdge - tmp;
  }

  tmp -= legRightToLeft;
  return leftEdge + tmp;
}

void PowderBotStripRenderer::resetScannerMotionToCenter() {
  scannerPos_ = (numPixels_ - 1) / 2;
  scannerDir_ = 1;
  scannerPrimed_ = true;
}

void PowderBotStripRenderer::stepScannerMotion() {
  if (!scannerPrimed_) {
    resetScannerMotionToCenter();
    return;
  }

  scannerPos_ += scannerDir_;

  if (scannerPos_ >= (int)numPixels_ - 1) {
    scannerPos_ = (int)numPixels_ - 1;
    scannerDir_ = -1;
  } else if (scannerPos_ <= 0) {
    scannerPos_ = 0;
    scannerDir_ = 1;
  }
}

void PowderBotStripRenderer::updateStartupVisual(uint32_t now, const PbRenderState &state) {
  const uint32_t elapsed = now - state.modeStartMs;
  if ((now - lastFrameMs_) < 20) return;
  lastFrameMs_ = now;

  const float t = (float)elapsed / (float)timing_.startupDurationMs;
  const int maxRadius = (numPixels_ / 2) - 1;

  if (t < 0.28f) {
    const float p = t / 0.28f;
    const float intensity = 0.08f + 0.42f * sinf(p * 3.14159f);
    fillSymmetricGradientNoShow(2, scaleColor(colorRed(255), intensity));
  } else if (t < 0.72f) {
    const float p = (t - 0.28f) / 0.44f;
    const uint32_t scannerElapsedMs = (uint32_t)(p * ((numPixels_ - 1) * 2 * timing_.scannerFrameMs));
    const int pos = computeCenteredScannerPos(scannerElapsedMs, timing_.scannerFrameMs);
    drawScannerNoShow(pos, 3, 220);
  } else {
    const float p = (t - 0.72f) / 0.28f;
    const int radius = (int)(maxRadius * p);
    const float intensity = 0.25f + 0.75f * p;
    fillSymmetricGradientNoShow(radius, scaleColor(colorRed(255), intensity));
  }

  showStrip();
}

void PowderBotStripRenderer::updateBleWakeVisual(uint32_t now, const PbRenderState &state) {
  const uint32_t elapsed = now - state.modeStartMs;
  if ((now - lastFrameMs_) < 20) return;
  lastFrameMs_ = now;

  const float t = (float)elapsed / (float)timing_.bleWakeDurationMs;
  const int maxRadius = (numPixels_ / 2) - 1;

  if (t < 0.52f) {
    const float p = t / 0.52f;
    const int radius = 1 + (int)(maxRadius * 0.55f * p);
    const float intensity = 0.10f + 0.80f * sinf(p * 3.14159f * 0.5f);
    fillSymmetricGradientNoShow(radius, scaleColor(colorBlue(255), intensity));
  } else {
    const float p = (t - 0.52f) / 0.48f;
    const int travel = (numPixels_ - 1) * 2;
    const int step = (int)(p * travel);
    const int pos = (step < (int)numPixels_) ? step : (travel - step);
    drawScannerColorNoShow(pos, 2, colorBlue(255), 210);
  }

  showStrip();
}

void PowderBotStripRenderer::updateDisconnectVisual(uint32_t now, const PbRenderState &state) {
  const uint32_t elapsed = now - state.modeStartMs;
  if ((now - lastFrameMs_) < 16) return;
  lastFrameMs_ = now;

  const float t = (float)elapsed / (float)timing_.bleDisconnectDurationMs;
  const int maxRadius = (numPixels_ / 2) - 1;
  const int radius = maxRadius - (int)(maxRadius * t);
  const float intensity = 0.70f * (1.0f - t) + 0.06f;

  fillSymmetricGradientNoShow(max(radius, 0), scaleColor(colorBlue(255), intensity));
  showStrip();
}

void PowderBotStripRenderer::updateStopAnimVisual(uint32_t now, const PbRenderState &state) {
  const uint32_t elapsed = now - state.modeStartMs;
  if ((now - lastFrameMs_) < 16) return;
  lastFrameMs_ = now;

  const float t = (float)elapsed / (float)timing_.stopAnimDurationMs;
  const int maxRadius = (numPixels_ / 2) - 1;
  const uint32_t stopAnimColor = verdictColor(state.stopVerdict);

  if (state.stopAnimStyle == PbStopAnimStyle::CenterPinch) {
    const float centerIntensity = 0.32f * (1.0f - t) + 0.08f;
    const float shoulderIntensity = 0.14f * (1.0f - t);
    const float outerShoulderIntensity = 0.06f * (1.0f - t);

    strip_.clear();
    const int leftCenter = (numPixels_ / 2) - 1;
    const int rightCenter = (numPixels_ / 2);

    const uint32_t center = scaleColor(stopAnimColor, centerIntensity);
    strip_.setPixelColor(leftCenter, center);
    strip_.setPixelColor(rightCenter, center);

    if (t < 0.72f) {
      const uint32_t shoulder = scaleColor(stopAnimColor, shoulderIntensity);
      if ((leftCenter - 1) >= 0) strip_.setPixelColor(leftCenter - 1, shoulder);
      if ((rightCenter + 1) < (int)numPixels_) strip_.setPixelColor(rightCenter + 1, shoulder);
    }

    if (t < 0.34f) {
      const uint32_t outerShoulder = scaleColor(stopAnimColor, outerShoulderIntensity);
      if ((leftCenter - 2) >= 0) strip_.setPixelColor(leftCenter - 2, outerShoulder);
      if ((rightCenter + 2) < (int)numPixels_) strip_.setPixelColor(rightCenter + 2, outerShoulder);
    }
  } else if (t < 0.68f) {
    const float p = t / 0.68f;
    const int radius = maxRadius - (int)(maxRadius * p);
    const float intensity = 0.20f + 0.75f * (1.0f - 0.55f * p);
    fillSymmetricGradientNoShow(max(radius, 0), scaleColor(stopAnimColor, intensity));
  } else {
    const float p = (t - 0.68f) / 0.32f;
    const float intensity = 0.42f * (1.0f - p) + 0.05f;

    strip_.clear();
    const int leftCenter = (numPixels_ / 2) - 1;
    const int rightCenter = (numPixels_ / 2);
    const uint32_t center = scaleColor(stopAnimColor, intensity);
    strip_.setPixelColor(leftCenter, center);
    strip_.setPixelColor(rightCenter, center);

    const uint32_t shoulder = scaleColor(stopAnimColor, intensity * 0.35f);
    if ((leftCenter - 1) >= 0) strip_.setPixelColor(leftCenter - 1, shoulder);
    if ((rightCenter + 1) < (int)numPixels_) strip_.setPixelColor(rightCenter + 1, shoulder);
  }

  showStrip();
}

void PowderBotStripRenderer::updateIdleEmberVisual(uint32_t now, const PbRenderState &state) {
  if ((now - lastFrameMs_) < 28) return;
  lastFrameMs_ = now;

  const float phase = (float)((now - state.modeStartMs) % timing_.idleEmberPeriodMs) / (float)timing_.idleEmberPeriodMs;
  const float pulse = 0.5f - 0.5f * cosf(phase * 6.28318f);
  const float intensity = 0.03f + 0.10f * pulse;

  strip_.clear();
  const int leftCenter = (numPixels_ / 2) - 1;
  const int rightCenter = (numPixels_ / 2);

  strip_.setPixelColor(leftCenter, scaleColor(colorRed(255), intensity));
  strip_.setPixelColor(rightCenter, scaleColor(colorRed(255), intensity));

  const uint32_t shoulder = scaleColor(colorRed(255), intensity * 0.45f);
  if ((leftCenter - 1) >= 0) strip_.setPixelColor(leftCenter - 1, shoulder);
  if ((rightCenter + 1) < (int)numPixels_) strip_.setPixelColor(rightCenter + 1, shoulder);

  showStrip();
}

void PowderBotStripRenderer::updateHeartbeatVisual(uint32_t now, const PbRenderState &state) {
  if ((now - lastFrameMs_) < 24) return;
  lastFrameMs_ = now;

  const float phase = (float)((now - state.modeStartMs) % timing_.heartbeatPeriodMs) / (float)timing_.heartbeatPeriodMs;
  const float pulseA = expf(-powf((phase - 0.18f) / 0.07f, 2.0f));
  const float pulseB = expf(-powf((phase - 0.34f) / 0.09f, 2.0f));
  const float intensity = 0.05f + 0.16f * pulseA + 0.34f * pulseB;
  const int radius = 1 + (pulseB > 0.28f ? 2 : 1);

  fillSymmetricGradientNoShow(radius, scaleColor(colorRed(255), intensity));
  showStrip();
}

void PowderBotStripRenderer::updateTargetAckVisual(uint32_t now, const PbRenderState &state) {
  const uint32_t elapsed = now - state.modeStartMs;
  if ((now - lastFrameMs_) < 22) return;
  lastFrameMs_ = now;

  const float t = (float)elapsed / (float)timing_.targetAckDurationMs;
  const int maxRadius = (numPixels_ / 2) - 1;

  if (t < 0.62f) {
    const float p = t / 0.62f;
    const int radius = (int)(maxRadius * p);
    const float intensity = 0.24f + 0.76f * p;
    fillSymmetricGradientNoShow(radius, scaleColor(colorRed(255), intensity));
  } else {
    const float p = (t - 0.62f) / 0.38f;
    const int radius = maxRadius - (int)(maxRadius * p);
    const float intensity = 0.22f + 0.45f * (1.0f - p);
    fillSymmetricGradientNoShow(radius, scaleColor(colorRed(255), intensity));
  }

  showStrip();
}

void PowderBotStripRenderer::updateScannerVisual(uint32_t now, const PbRenderState &) {
  if (!scannerPrimed_) {
    resetScannerMotionToCenter();
  }

  if (lastFrameMs_ == 0) {
    lastFrameMs_ = now;
    drawScannerNoShow(scannerPos_, 3, 255);
    showStrip();
    return;
  }

  if ((now - lastFrameMs_) < timing_.scannerFrameMs) return;
  lastFrameMs_ = now;

  stepScannerMotion();
  drawScannerNoShow(scannerPos_, 3, 255);
  showStrip();
}

static void pbFillCenterOutReveal(Adafruit_NeoPixel &strip, uint16_t numPixels, uint16_t revealedPairs, uint32_t color) {
  strip.clear();

  if (numPixels == 0 || revealedPairs == 0) {
    return;
  }

  const bool evenCount = (numPixels % 2U) == 0U;
  const int leftCenter = (numPixels / 2) - 1;
  const int rightCenter = evenCount ? (numPixels / 2) : (numPixels / 2);
  const uint16_t maxPairs = evenCount ? (numPixels / 2) : ((numPixels + 1) / 2);
  const uint16_t clampedPairs = (revealedPairs > maxPairs) ? maxPairs : revealedPairs;

  for (uint16_t step = 0; step < clampedPairs; ++step) {
    const int left = leftCenter - (int)step;
    const int right = rightCenter + (int)step;

    if (left >= 0 && left < (int)numPixels) {
      strip.setPixelColor(left, color);
    }
    if (right >= 0 && right < (int)numPixels) {
      strip.setPixelColor(right, color);
    }
  }
}

void PowderBotStripRenderer::updatePassAnimVisual(uint32_t now, const PbRenderState &state) {
  const uint32_t elapsed = now - state.modeStartMs;
  if ((now - lastFrameMs_) < timing_.resetWipeStepMs) return;
  lastFrameMs_ = now;

  const uint16_t revealedPairs = elapsed / timing_.resetWipeStepMs;
  pbFillCenterOutReveal(strip_, numPixels_, revealedPairs, colorGreen(255));
  showStrip();
}

void PowderBotStripRenderer::updateFailAnimVisual(uint32_t now, const PbRenderState &state) {
  const uint32_t elapsed = now - state.modeStartMs;
  if ((now - lastFrameMs_) < timing_.resetWipeStepMs) return;
  lastFrameMs_ = now;

  const uint16_t revealedPairs = elapsed / timing_.resetWipeStepMs;
  pbFillCenterOutReveal(strip_, numPixels_, revealedPairs, colorRed(255));
  showStrip();
}

void PowderBotStripRenderer::updateSolidHoldVisual(uint32_t color, uint32_t now) {
  if ((now - lastFrameMs_) < 40) return;
  lastFrameMs_ = now;
  fillAllNoShow(color);
  showStrip();
}

void PowderBotStripRenderer::updateResetWipeVisual(uint32_t now, const PbRenderState &state) {
  const uint32_t elapsed = now - state.modeStartMs;
  if ((now - lastFrameMs_) < timing_.resetWipeStepMs) return;
  lastFrameMs_ = now;

  const uint16_t extinguishedPairs = elapsed / timing_.resetWipeStepMs;
  fillAllNoShow(verdictColor(state.wipeVerdict));

  for (uint16_t step = 0; step < extinguishedPairs; ++step) {
    const int left = step;
    const int right = numPixels_ - 1 - step;
    if (left >= 0 && left < (int)numPixels_) strip_.setPixelColor(left, colorOff());
    if (right >= 0 && right < (int)numPixels_) strip_.setPixelColor(right, colorOff());
  }

  showStrip();
}

void PowderBotStripRenderer::render(const PbRenderState &state, uint32_t now) {
  if (state.mode != lastMode_) {
    lastMode_ = state.mode;
    lastFrameMs_ = 0;
    if (state.mode == PbVisualMode::Scanner) {
      resetScannerMotionToCenter();
    }
  }

  switch (state.mode) {
    case PbVisualMode::Off:
      clearStripNoShow();
      showStrip();
      break;
    case PbVisualMode::Startup:
      updateStartupVisual(now, state);
      break;
    case PbVisualMode::BleWake:
      updateBleWakeVisual(now, state);
      break;
    case PbVisualMode::BleDisconnect:
      updateDisconnectVisual(now, state);
      break;
    case PbVisualMode::StopAnim:
      updateStopAnimVisual(now, state);
      break;
    case PbVisualMode::IdleEmber:
      updateIdleEmberVisual(now, state);
      break;
    case PbVisualMode::Heartbeat:
      updateHeartbeatVisual(now, state);
      break;
    case PbVisualMode::TargetAck:
      updateTargetAckVisual(now, state);
      break;
    case PbVisualMode::Scanner:
      updateScannerVisual(now, state);
      break;
    case PbVisualMode::PassAnim:
      updatePassAnimVisual(now, state);
      break;
    case PbVisualMode::PassHold:
      updateSolidHoldVisual(colorGreen(255), now);
      break;
    case PbVisualMode::FailAnim:
      updateFailAnimVisual(now, state);
      break;
    case PbVisualMode::FailHold:
      updateSolidHoldVisual(colorRed(255), now);
      break;
    case PbVisualMode::ResetWipe:
      updateResetWipeVisual(now, state);
      break;
  }
}
