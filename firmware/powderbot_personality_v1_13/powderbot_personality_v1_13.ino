/*
  PowderBot 9000 - working powder-throw sketch with personality animations
  Board: JC2432W328C (ESP32 Dev Module)

  Purpose:
  - Sniff BLE TX on GPIO21 for target lines like "T70.0"
  - Read BLE STATE on GPIO22 for real connect/disconnect events
  - Sniff scale stream on GPIO35 for lines like:
      ST,+00070.00 GN
      US,+00069.96 GN
  - Drive an external addressable LED strip on GPIO17

  Visual behavior:
    STARTUP           = short wake-up scan on boot
    BLE WAKE          = blue pulse / scanner wake-up (on BLE STATE connect)
    BLE DISCONNECT    = blue collapse when BLE STATE goes low
    BLE READY         = soft red center ember while app/BLE is connected but no target is loaded
    ARMED / WAITING   = stronger red heartbeat once a target is set
    TARGET RECEIVED   = quick red center-out acknowledgment
    THROW IN PROGRESS = red Knight Rider scan once a real run is underway
    PASS              = green bloom, then solid green while within tolerance
    FAIL              = red pulse, then solid red while outside tolerance
    CUP REMOVED       = edge-to-center wipe to black, then re-arm same target
    BLE STOP / DISARM = current truth color collapses edge-to-center (green if pass, red otherwise), then return to BLE READY ember if still connected
    NO TARGET         = off until BLE/app activity is first observed

  Decision behavior:
  - Do NOT decide from one lone ST line
  - Require a rolling window of agreeing ST samples near target
  - Verdict is based on the average of that rolling sample window
  - Verdict can change GREEN <-> RED if the settled average drifts across tolerance
  - Heartbeat begins once target is set
  - Scanner begins only after a real run is latched
  - Once a result is active, cup-removal readings are ignored for verdict updates and only used for reset detection

  Hardware:
    GPIO17 -> 330 ohm resistor -> strip DI
    ESP32 GND tied to strip/power supply GND
    5V supply -> strip +5V

  Requires:
    - EspSoftwareSerial library
    - Adafruit NeoPixel library
*/

#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>

// -------------------------
// Pin map
// -------------------------
static const int BLE_RX_PIN      = 21;   // BLE module TX -> ESP32 RX
static const int BLE_STATE_PIN   = 22;   // BLE STATE line: LOW=disconnected, HIGH=connected
static const int SCALE_RX_PIN    = 35;   // MAX3232 R1OUT -> scale stream
static const int STRIP_DATA_PIN  = 17;   // external strip data pin

// -------------------------
// LED strip config
// -------------------------
static const uint16_t NUM_PIXELS = 22;

// *** USER ADJUST THIS LINE FOR LED BRIGHTNESS ***
static const uint8_t LED_BRIGHTNESS = 100;

// *** USER ADJUST THIS LINE FOR STRIP PERSONALITY MODE ***
// true  = full personality show (startup / BLE / idle / scanner / pass / fail / reset)
// false = minimal mode (green/red result visuals + same-color cup-removal wipe)
static const bool FULL_SHOW = true;

Adafruit_NeoPixel strip(NUM_PIXELS, STRIP_DATA_PIN, NEO_GRB + NEO_KHZ800);

// -------------------------
// Serial settings
// -------------------------
static const uint32_t USB_BAUD      = 115200;
static const uint32_t BLE_BAUD      = 9600;
static const uint32_t SCALE_BAUD    = 19200;
static const uint32_t FRAME_GAP_MS  = 20;
static const uint32_t BLE_STALE_TIMEOUT_MS   = 1500;
static const uint32_t SCALE_STALE_TIMEOUT_MS = 1500;
static const bool ENABLE_BLE_STALE_WARNINGS   = false;
static const bool ENABLE_SCALE_STALE_WARNINGS = true;

// -------------------------
// Logic tuning
// -------------------------
// *** USER ADJUST THIS LINE FOR PASS / FAIL TOLERANCE ***
static const float FINAL_TOLERANCE_GN       = 0.02f;
static const float ARM_BELOW_TARGET_GN      = 0.08f;   // don't start final-decision window until this close
static const uint8_t DECISION_SAMPLES       = 4;       // agreeing stable samples required
static const float DECISION_MAX_SPREAD_GN   = 0.04f;   // max min-to-max span allowed in sample window
static const float RUN_START_THRESHOLD_GN   = 1.00f;   // treat the run as genuinely underway above this weight
static const uint8_t RUN_START_COUNT         = 2;       // consecutive above-threshold readings required to latch scanner
static const float CUP_REMOVAL_THRESHOLD_GN = 1.00f;   // treat cup as removed below this weight once a result exists
static const uint8_t CUP_REMOVAL_COUNT      = 3;       // consecutive low readings required to reset run
static const float TARGET_DUPLICATE_EPSILON_GN = 0.0005f; // treat repeated BLE target packets as the same target within this tolerance

// -------------------------
// BLE STATE debounce
// -------------------------
static const uint8_t INPUT_MODE = INPUT;
static const uint32_t STATE_DEBOUNCE_MS = 30;

// -------------------------
// Animation timing
// -------------------------
static const uint16_t STARTUP_DURATION_MS       = 850;
static const uint16_t BLE_WAKE_DURATION_MS      = 620;
static const uint16_t BLE_DISCONNECT_DURATION_MS = 340;
static const uint16_t STOP_ANIM_DURATION_MS      = 340;
static const uint16_t IDLE_EMBER_PERIOD_MS      = 2000;
static const uint16_t HEARTBEAT_PERIOD_MS       = 1400;
static const uint16_t TARGET_ACK_DURATION_MS    = 520;
static const uint16_t SCANNER_FRAME_MS          = 90;
static const uint16_t PASS_ANIM_DURATION_MS     = 260;
static const uint16_t FAIL_ANIM_DURATION_MS     = 260;
static const uint16_t RESET_WIPE_STEP_MS        = 36;

// RX-only UARTs
SoftwareSerial BLE_UART(BLE_RX_PIN, -1, false);
HardwareSerial SCALE_UART(1);

// -------------------------
// Line buffers
// -------------------------
char bleBuf[160];
size_t bleLen = 0;
uint32_t lastBleByteTime = 0;
bool bleOverflowed = false;
bool bleTrafficSeen = false;
bool bleStaleReported = false;

char scaleBuf[160];
size_t scaleLen = 0;
uint32_t lastScaleByteTime = 0;
bool scaleOverflowed = false;
bool scaleTrafficSeen = false;
bool scaleStaleReported = false;

// -------------------------
// State
// -------------------------
float targetWeightGn = 0.0f;
bool targetValid = false;

float liveWeightGn = 0.0f;
bool liveStable = false;

float stableWindow[DECISION_SAMPLES];
uint8_t stableWindowCount = 0;
bool decisionActive = false;
float displayedWeightGn = 0.0f;           // latest settled rolling-window average used for verdict
uint8_t cupRemovalCounter = 0;

// -------------------------
// Verdict state
// -------------------------
enum LedState {
  LED_STATE_OFF,
  LED_STATE_GREEN,
  LED_STATE_RED
};

LedState lastLoggedVerdictState = LED_STATE_OFF;
LedState lastAnimatedVerdict = LED_STATE_OFF;

// -------------------------
// Visual mode state machine
// -------------------------
enum VisualMode {
  VISUAL_OFF,
  VISUAL_STARTUP,
  VISUAL_BLE_WAKE,
  VISUAL_BLE_DISCONNECT,
  VISUAL_STOP_ANIM,
  VISUAL_IDLE_EMBER,
  VISUAL_HEARTBEAT,
  VISUAL_TARGET_ACK,
  VISUAL_SCANNER,
  VISUAL_PASS_ANIM,
  VISUAL_PASS_HOLD,
  VISUAL_FAIL_ANIM,
  VISUAL_FAIL_HOLD,
  VISUAL_RESET_WIPE
};

VisualMode visualMode = VISUAL_OFF;
uint32_t visualModeStartMs = 0;
uint32_t lastVisualFrameMs = 0;
uint32_t resetWipeColor = 0;
uint32_t stopAnimColor = 0;
enum StopAnimStyle {
  STOP_ANIM_FULL_COLLAPSE,
  STOP_ANIM_CENTER_PINCH
};
StopAnimStyle stopAnimStyle = STOP_ANIM_FULL_COLLAPSE;
bool startupPending = true;
bool bleWakePending = false;
bool bleDisconnectPending = false;
bool stopAnimPending = false;
bool targetAckPending = false;
bool resetWipePending = false;

// Scanner motion is persistent and event-reset, not derived from visualModeStartMs.
int scannerPos = (NUM_PIXELS - 1) / 2;
int scannerDir = 1;
bool scannerPrimed = false;

// Throw visual phase is latched by real run events, not by re-evaluating thresholds every frame.
// false = armed heartbeat / waiting
// true  = active throw scanner until stable verdict logic takes over
bool runVisualLatched = false;
uint8_t runStartCounter = 0;

// -------------------------
// BLE state tracking
// -------------------------
bool bleConnected = false;         // debounced stable state
bool stateRaw = false;
bool stateCandidate = false;
uint32_t stateCandidateSinceMs = 0;

// -------------------------
// Color helpers
// -------------------------
uint32_t colorOff()                { return strip.Color(0, 0, 0); }
uint32_t colorRed(uint8_t level)   { return strip.Color(level, 0, 0); }
uint32_t colorGreen(uint8_t level) { return strip.Color(0, level, 0); }
uint32_t colorBlue(uint8_t level)  { return strip.Color(0, 0, level); }

uint32_t scaleColor(uint32_t color, float scale) {
  scale = constrain(scale, 0.0f, 1.0f);
  uint8_t r = (uint8_t)(((color >> 16) & 0xFF) * scale);
  uint8_t g = (uint8_t)(((color >>  8) & 0xFF) * scale);
  uint8_t b = (uint8_t)(( color        & 0xFF) * scale);
  return strip.Color(r, g, b);
}

void clearStripNoShow() {
  strip.clear();
}

void showStrip() {
  strip.show();
}

void fillAllNoShow(uint32_t color) {
  for (uint16_t i = 0; i < NUM_PIXELS; i++) {
    strip.setPixelColor(i, color);
  }
}

void fillSymmetricGradientNoShow(int radius, uint32_t color) {
  int leftCenter  = (NUM_PIXELS / 2) - 1;
  int rightCenter = (NUM_PIXELS / 2);

  strip.clear();
  for (int d = 0; d <= radius; d++) {
    float scale = 0.35f + 0.65f * ((float)(radius - d + 1) / (float)(radius + 1));
    uint32_t c = scaleColor(color, scale);
    int left  = leftCenter - d;
    int right = rightCenter + d;
    if (left >= 0) strip.setPixelColor(left, c);
    if (right < (int)NUM_PIXELS) strip.setPixelColor(right, c);
  }
}

void drawScannerColorNoShow(int pos, uint8_t tail, uint32_t color, uint8_t maxLevel) {
  strip.clear();
  for (int i = 0; i < (int)NUM_PIXELS; i++) {
    int dist = abs(i - pos);
    if (dist <= tail) {
      float scale = 1.0f - ((float)dist / (float)(tail + 1));
      scale *= scale;
      strip.setPixelColor(i, scaleColor(color, ((float)maxLevel / 255.0f) * scale));
    }
  }
}

void setPixelColorClamped(int index, uint32_t color) {
  if (index < 0 || index >= (int)NUM_PIXELS) return;
  strip.setPixelColor(index, color);
}

void drawScannerNoShow(int pos, uint8_t tail, uint8_t maxLevel) {
  (void)tail;  // CHANGE: fixed 5-pixel KITT render to match the standalone scanner shape

  strip.clear();

  uint8_t headLevel  = maxLevel;
  uint8_t innerLevel = (uint8_t)((maxLevel * 90U) / 255U);
  uint8_t outerLevel = (uint8_t)((maxLevel * 25U) / 255U);

  setPixelColorClamped(pos - 2, colorRed(outerLevel));
  setPixelColorClamped(pos - 1, colorRed(innerLevel));
  setPixelColorClamped(pos,     colorRed(headLevel));
  setPixelColorClamped(pos + 1, colorRed(innerLevel));
  setPixelColorClamped(pos + 2, colorRed(outerLevel));
}

int computeCenteredScannerPos(uint32_t elapsedMs, uint16_t frameMs) {
  int centerPos = (NUM_PIXELS - 1) / 2;  // CHANGE: start the single-pixel scanner from the visual center on even-length strips
  int rightEdge = NUM_PIXELS - 1;
  int leftEdge = 0;

  int legCenterToRight = rightEdge - centerPos;
  int legRightToLeft = rightEdge - leftEdge;
  int legLeftToCenter = centerPos - leftEdge;
  int totalTravel = legCenterToRight + legRightToLeft + legLeftToCenter;

  int step = (int)((elapsedMs / frameMs) % totalTravel);

  if (step <= legCenterToRight) {
    return centerPos + step;
  }

  step -= legCenterToRight;
  if (step <= legRightToLeft) {
    return rightEdge - step;
  }

  step -= legRightToLeft;
  return leftEdge + step;
}

void resetScannerMotionToCenter() {
  scannerPos = (NUM_PIXELS - 1) / 2;
  scannerDir = 1;
  scannerPrimed = true;
}

void stepScannerMotion() {
  if (!scannerPrimed) {
    resetScannerMotionToCenter();
    return;
  }

  scannerPos += scannerDir;

  if (scannerPos >= (int)NUM_PIXELS - 1) {
    scannerPos = (int)NUM_PIXELS - 1;
    scannerDir = -1;
  } else if (scannerPos <= 0) {
    scannerPos = 0;
    scannerDir = 1;
  }
}

const char *visualModeName(VisualMode mode) {
  switch (mode) {
    case VISUAL_OFF:        return "OFF";
    case VISUAL_STARTUP:        return "STARTUP";
    case VISUAL_BLE_WAKE:       return "BLE WAKE";
    case VISUAL_BLE_DISCONNECT: return "BLE DISCONNECT";
    case VISUAL_STOP_ANIM:      return "STOP ANIM";
    case VISUAL_IDLE_EMBER:     return "IDLE EMBER";
    case VISUAL_HEARTBEAT:      return "HEARTBEAT";
    case VISUAL_TARGET_ACK: return "TARGET ACK";
    case VISUAL_SCANNER:    return "SCANNER";
    case VISUAL_PASS_ANIM:  return "PASS ANIM";
    case VISUAL_PASS_HOLD:  return "PASS HOLD";
    case VISUAL_FAIL_ANIM:  return "FAIL ANIM";
    case VISUAL_FAIL_HOLD:  return "FAIL HOLD";
    case VISUAL_RESET_WIPE: return "RESET WIPE";
  }
  return "?";
}

void beginVisualMode(VisualMode newMode, bool forceRestart = false) {
  if (!forceRestart && newMode == visualMode) return;
  visualMode = newMode;
  visualModeStartMs = millis();
  lastVisualFrameMs = 0;

  Serial.print("VISUAL -> ");
  Serial.println(visualModeName(newMode));
}

LedState getDesiredVerdictState() {
  if (!targetValid || !decisionActive) return LED_STATE_OFF;

  float errorGn = displayedWeightGn - targetWeightGn;
  if (fabsf(errorGn) <= FINAL_TOLERANCE_GN) {
    return LED_STATE_GREEN;
  }
  return LED_STATE_RED;
}

const char *ledStateName(LedState state) {
  switch (state) {
    case LED_STATE_OFF:   return "OFF";
    case LED_STATE_GREEN: return "GREEN";
    case LED_STATE_RED:   return "RED";
  }
  return "?";
}

void logVerdictTransition() {
  LedState newVerdict = getDesiredVerdictState();
  if (newVerdict == lastLoggedVerdictState) return;

  lastLoggedVerdictState = newVerdict;
  Serial.print("VERDICT -> ");
  Serial.println(ledStateName(lastLoggedVerdictState));
}

VisualMode computeBaseVisualMode() {
  if (!bleConnected) {
    return VISUAL_OFF;
  }

  if (!targetValid) {
    return VISUAL_IDLE_EMBER;
  }

  if (decisionActive) {
    return (getDesiredVerdictState() == LED_STATE_GREEN) ? VISUAL_PASS_HOLD : VISUAL_FAIL_HOLD;
  }

  // Armed flow, but latched by real run state instead of per-sample threshold ping-pong:
  // - heartbeat after target set while waiting for the throw to actually start
  // - scanner once the cup/load is really present and the throw has begun
  // - stay in scanner until the settled verdict logic takes over
  if (!runVisualLatched) {
    return VISUAL_HEARTBEAT;
  }

  return VISUAL_SCANNER;
}

bool updateStartupVisual(uint32_t now) {
  uint32_t elapsed = now - visualModeStartMs;
  if (elapsed >= STARTUP_DURATION_MS) {
    clearStripNoShow();
    showStrip();
    return true;
  }

  if ((now - lastVisualFrameMs) < 20) return false;
  lastVisualFrameMs = now;

  float t = (float)elapsed / (float)STARTUP_DURATION_MS;
  int maxRadius = (NUM_PIXELS / 2) - 1;

  if (t < 0.28f) {
    float p = t / 0.28f;
    float intensity = 0.08f + 0.42f * sinf(p * 3.14159f);
    fillSymmetricGradientNoShow(2, scaleColor(colorRed(255), intensity));
  } else if (t < 0.72f) {
    float p = (t - 0.28f) / 0.44f;
    uint32_t scannerElapsedMs = (uint32_t)(p * ((NUM_PIXELS - 1) * 2 * SCANNER_FRAME_MS));
    int pos = computeCenteredScannerPos(scannerElapsedMs, SCANNER_FRAME_MS);
    drawScannerNoShow(pos, 3, 220);
  } else {
    float p = (t - 0.72f) / 0.28f;
    int radius = (int)(maxRadius * p);
    float intensity = 0.25f + 0.75f * p;
    fillSymmetricGradientNoShow(radius, scaleColor(colorRed(255), intensity));
  }

  showStrip();
  return false;
}

bool updateBleWakeVisual(uint32_t now) {
  uint32_t elapsed = now - visualModeStartMs;
  if (elapsed >= BLE_WAKE_DURATION_MS) {
    clearStripNoShow();
    showStrip();
    return true;
  }

  if ((now - lastVisualFrameMs) < 20) return false;
  lastVisualFrameMs = now;

  float t = (float)elapsed / (float)BLE_WAKE_DURATION_MS;
  int maxRadius = (NUM_PIXELS / 2) - 1;

  if (t < 0.52f) {
    float p = t / 0.52f;
    int radius = 1 + (int)(maxRadius * 0.55f * p);
    float intensity = 0.10f + 0.80f * sinf(p * 3.14159f * 0.5f);
    fillSymmetricGradientNoShow(radius, scaleColor(colorBlue(255), intensity));
  } else {
    float p = (t - 0.52f) / 0.48f;
    int travel = (NUM_PIXELS - 1) * 2;
    int step = (int)(p * travel);
    int pos = (step < (int)NUM_PIXELS) ? step : (travel - step);
    drawScannerColorNoShow(pos, 2, colorBlue(255), 210);
  }

  showStrip();
  return false;
}

bool updateDisconnectVisual(uint32_t now) {
  uint32_t elapsed = now - visualModeStartMs;
  if (elapsed >= BLE_DISCONNECT_DURATION_MS) {
    clearStripNoShow();
    showStrip();
    return true;
  }

  if ((now - lastVisualFrameMs) < 16) return false;
  lastVisualFrameMs = now;

  float t = (float)elapsed / (float)BLE_DISCONNECT_DURATION_MS;
  int maxRadius = (NUM_PIXELS / 2) - 1;
  int radius = maxRadius - (int)(maxRadius * t);
  float intensity = 0.70f * (1.0f - t) + 0.06f;

  fillSymmetricGradientNoShow(max(radius, 0), scaleColor(colorBlue(255), intensity));
  showStrip();
  return false;
}

uint32_t getDisarmAnimColor() {
  LedState disarmState = getDesiredVerdictState();
  if (disarmState == LED_STATE_GREEN) return colorGreen(255);
  return colorRed(255);
}

bool updateStopAnimVisual(uint32_t now) {
  uint32_t elapsed = now - visualModeStartMs;
  if (elapsed >= STOP_ANIM_DURATION_MS) {
    clearStripNoShow();
    showStrip();
    return true;
  }

  if ((now - lastVisualFrameMs) < 16) return false;
  lastVisualFrameMs = now;

  float t = (float)elapsed / (float)STOP_ANIM_DURATION_MS;
  int maxRadius = (NUM_PIXELS / 2) - 1;

  if (stopAnimStyle == STOP_ANIM_CENTER_PINCH) {
    // CHANGE: no-result disarm starts from the center family so it does not flash wide first
    float centerIntensity = 0.32f * (1.0f - t) + 0.08f;
    float shoulderIntensity = 0.14f * (1.0f - t);
    float outerShoulderIntensity = 0.06f * (1.0f - t);

    strip.clear();
    int leftCenter  = (NUM_PIXELS / 2) - 1;
    int rightCenter = (NUM_PIXELS / 2);

    uint32_t center = scaleColor(stopAnimColor, centerIntensity);
    strip.setPixelColor(leftCenter, center);
    strip.setPixelColor(rightCenter, center);

    if (t < 0.72f) {
      uint32_t shoulder = scaleColor(stopAnimColor, shoulderIntensity);
      if ((leftCenter - 1) >= 0) strip.setPixelColor(leftCenter - 1, shoulder);
      if ((rightCenter + 1) < (int)NUM_PIXELS) strip.setPixelColor(rightCenter + 1, shoulder);
    }

    if (t < 0.34f) {
      uint32_t outerShoulder = scaleColor(stopAnimColor, outerShoulderIntensity);
      if ((leftCenter - 2) >= 0) strip.setPixelColor(leftCenter - 2, outerShoulder);
      if ((rightCenter + 2) < (int)NUM_PIXELS) strip.setPixelColor(rightCenter + 2, outerShoulder);
    }
  } else if (t < 0.68f) {
    // CHANGE: reverse of target-ack geometry: edges collapse inward toward center
    float p = t / 0.68f;
    int radius = maxRadius - (int)(maxRadius * p);
    float intensity = 0.20f + 0.75f * (1.0f - 0.55f * p);
    fillSymmetricGradientNoShow(max(radius, 0), scaleColor(stopAnimColor, intensity));
  } else {
    // CHANGE: brief center pinch / fade before handing off to ember or off
    float p = (t - 0.68f) / 0.32f;
    float intensity = 0.42f * (1.0f - p) + 0.05f;

    strip.clear();
    int leftCenter  = (NUM_PIXELS / 2) - 1;
    int rightCenter = (NUM_PIXELS / 2);
    uint32_t center = scaleColor(stopAnimColor, intensity);
    strip.setPixelColor(leftCenter, center);
    strip.setPixelColor(rightCenter, center);

    uint32_t shoulder = scaleColor(stopAnimColor, intensity * 0.35f);
    if ((leftCenter - 1) >= 0) strip.setPixelColor(leftCenter - 1, shoulder);
    if ((rightCenter + 1) < (int)NUM_PIXELS) strip.setPixelColor(rightCenter + 1, shoulder);
  }

  showStrip();
  return false;
}

bool updateIdleEmberVisual(uint32_t now) {
  if ((now - lastVisualFrameMs) < 28) return false;
  lastVisualFrameMs = now;

  float phase = (float)((now - visualModeStartMs) % IDLE_EMBER_PERIOD_MS) / (float)IDLE_EMBER_PERIOD_MS;
  float pulse = 0.5f - 0.5f * cosf(phase * 6.28318f);
  float intensity = 0.03f + 0.10f * pulse;

  strip.clear();
  int leftCenter  = (NUM_PIXELS / 2) - 1;
  int rightCenter = (NUM_PIXELS / 2);

  strip.setPixelColor(leftCenter,  scaleColor(colorRed(255), intensity));
  strip.setPixelColor(rightCenter, scaleColor(colorRed(255), intensity));

  uint32_t shoulder = scaleColor(colorRed(255), intensity * 0.45f);
  if ((leftCenter - 1) >= 0) strip.setPixelColor(leftCenter - 1, shoulder);
  if ((rightCenter + 1) < (int)NUM_PIXELS) strip.setPixelColor(rightCenter + 1, shoulder);

  showStrip();
  return false;
}

bool updateHeartbeatVisual(uint32_t now) {
  if ((now - lastVisualFrameMs) < 24) return false;
  lastVisualFrameMs = now;

  float phase = (float)((now - visualModeStartMs) % HEARTBEAT_PERIOD_MS) / (float)HEARTBEAT_PERIOD_MS;
  float pulseA = expf(-powf((phase - 0.18f) / 0.07f, 2.0f));
  float pulseB = expf(-powf((phase - 0.34f) / 0.09f, 2.0f));
  float intensity = 0.05f + 0.16f * pulseA + 0.34f * pulseB;
  int radius = 1 + (pulseB > 0.28f ? 2 : 1);

  fillSymmetricGradientNoShow(radius, scaleColor(colorRed(255), intensity));
  showStrip();
  return false;
}

bool updateTargetAckVisual(uint32_t now) {
  uint32_t elapsed = now - visualModeStartMs;
  if (elapsed >= TARGET_ACK_DURATION_MS) {
    clearStripNoShow();
    showStrip();
    return true;
  }

  if ((now - lastVisualFrameMs) < 22) return false;
  lastVisualFrameMs = now;

  float t = (float)elapsed / (float)TARGET_ACK_DURATION_MS;
  int maxRadius = (NUM_PIXELS / 2) - 1;

  if (t < 0.62f) {
    float p = t / 0.62f;
    int radius = (int)(maxRadius * p);
    float intensity = 0.24f + 0.76f * p;
    fillSymmetricGradientNoShow(radius, scaleColor(colorRed(255), intensity));
  } else {
    float p = (t - 0.62f) / 0.38f;
    int radius = maxRadius - (int)(maxRadius * p);
    float intensity = 0.22f + 0.45f * (1.0f - p);
    fillSymmetricGradientNoShow(radius, scaleColor(colorRed(255), intensity));
  }

  showStrip();
  return false;
}

bool updateScannerVisual(uint32_t now) {
  if (!scannerPrimed) {
    resetScannerMotionToCenter();
  }

  if (lastVisualFrameMs == 0) {
    lastVisualFrameMs = now;
    drawScannerNoShow(scannerPos, 3, 255);
    showStrip();
    return false;
  }

  if ((now - lastVisualFrameMs) < SCANNER_FRAME_MS) return false;
  lastVisualFrameMs = now;

  stepScannerMotion();
  drawScannerNoShow(scannerPos, 3, 255);
  showStrip();
  return false;
}

bool updatePassAnimVisual(uint32_t now) {
  uint32_t elapsed = now - visualModeStartMs;
  if (elapsed >= PASS_ANIM_DURATION_MS) {
    fillAllNoShow(colorGreen(255));
    showStrip();
    return true;
  }

  if ((now - lastVisualFrameMs) < 20) return false;
  lastVisualFrameMs = now;

  float t = (float)elapsed / (float)PASS_ANIM_DURATION_MS;
  int maxRadius = (NUM_PIXELS / 2) - 1;
  int radius = (int)(maxRadius * t);
  float intensity = 0.20f + 0.80f * t;

  fillSymmetricGradientNoShow(radius, scaleColor(colorGreen(255), intensity));
  showStrip();
  return false;
}

bool updateFailAnimVisual(uint32_t now) {
  uint32_t elapsed = now - visualModeStartMs;
  if (elapsed >= FAIL_ANIM_DURATION_MS) {
    fillAllNoShow(colorRed(255));
    showStrip();
    return true;
  }

  if ((now - lastVisualFrameMs) < 18) return false;
  lastVisualFrameMs = now;

  float t = (float)elapsed / (float)FAIL_ANIM_DURATION_MS;
  float envelope;
  if (t < 0.55f) {
    envelope = 0.18f + 0.82f * (t / 0.55f);
  } else {
    envelope = 0.72f + 0.28f * ((t - 0.55f) / 0.45f);
  }

  fillAllNoShow(scaleColor(colorRed(255), envelope));
  showStrip();
  return false;
}

bool updateSolidHoldVisual(uint32_t color) {
  if ((millis() - lastVisualFrameMs) < 40) return false;
  lastVisualFrameMs = millis();
  fillAllNoShow(color);
  showStrip();
  return false;
}

bool updateResetWipeVisual(uint32_t now) {
  uint16_t pairs = NUM_PIXELS / 2;
  uint32_t totalDuration = (pairs + 1) * RESET_WIPE_STEP_MS;
  uint32_t elapsed = now - visualModeStartMs;

  if (elapsed >= totalDuration) {
    clearStripNoShow();
    showStrip();
    return true;
  }

  if ((now - lastVisualFrameMs) < RESET_WIPE_STEP_MS) return false;
  lastVisualFrameMs = now;

  uint16_t extinguishedPairs = elapsed / RESET_WIPE_STEP_MS;
  fillAllNoShow(resetWipeColor);

  for (uint16_t step = 0; step < extinguishedPairs; step++) {
    int left = step;
    int right = NUM_PIXELS - 1 - step;
    if (left >= 0 && left < (int)NUM_PIXELS) strip.setPixelColor(left, colorOff());
    if (right >= 0 && right < (int)NUM_PIXELS) strip.setPixelColor(right, colorOff());
  }

  showStrip();
  return false;
}

VisualMode computeMinimalVisualMode() {
  if (!bleConnected) return VISUAL_OFF;
  if (!targetValid || !decisionActive) return VISUAL_OFF;
  return (getDesiredVerdictState() == LED_STATE_GREEN) ? VISUAL_PASS_HOLD : VISUAL_FAIL_HOLD;
}

void suppressNonVerdictPendingFlags() {
  startupPending = false;
  bleWakePending = false;
  bleDisconnectPending = false;
  stopAnimPending = false;
  targetAckPending = false;
}

void updateVisuals() {
  uint32_t now = millis();

  if (!FULL_SHOW) {
    suppressNonVerdictPendingFlags();

    // Keep the same-color cup-removal exit wipe even in minimal mode.
    if (resetWipePending || visualMode == VISUAL_RESET_WIPE) {
      if (visualMode != VISUAL_RESET_WIPE) {
        beginVisualMode(VISUAL_RESET_WIPE, true);
      }
      if (updateResetWipeVisual(now)) {
        resetWipePending = false;
        beginVisualMode(computeMinimalVisualMode(), true);
      }
      return;
    }

    // Still allow the real verdict transition animations, but suppress the rest of the theater.
    LedState desiredVerdict = getDesiredVerdictState();
    if (desiredVerdict == LED_STATE_GREEN && lastAnimatedVerdict != LED_STATE_GREEN) {
      lastAnimatedVerdict = LED_STATE_GREEN;
      beginVisualMode(VISUAL_PASS_ANIM, true);
    } else if (desiredVerdict == LED_STATE_RED && lastAnimatedVerdict != LED_STATE_RED) {
      lastAnimatedVerdict = LED_STATE_RED;
      beginVisualMode(VISUAL_FAIL_ANIM, true);
    } else if (desiredVerdict == LED_STATE_OFF) {
      lastAnimatedVerdict = LED_STATE_OFF;
    }

    if (visualMode == VISUAL_PASS_ANIM) {
      if (updatePassAnimVisual(now)) {
        beginVisualMode(VISUAL_PASS_HOLD, true);
      }
      return;
    }

    if (visualMode == VISUAL_FAIL_ANIM) {
      if (updateFailAnimVisual(now)) {
        beginVisualMode(VISUAL_FAIL_HOLD, true);
      }
      return;
    }

    VisualMode baseMode = computeMinimalVisualMode();
    if (visualMode != baseMode) {
      beginVisualMode(baseMode, true);
    }

    switch (visualMode) {
      case VISUAL_OFF:
        clearStripNoShow();
        showStrip();
        break;
      case VISUAL_PASS_HOLD:
        updateSolidHoldVisual(colorGreen(255));
        break;
      case VISUAL_FAIL_HOLD:
        updateSolidHoldVisual(colorRed(255));
        break;
      default:
        clearStripNoShow();
        showStrip();
        break;
    }
    return;
  }

  // Let target capture immediately cancel the little boot show.
  if (targetValid) {
    startupPending = false;
  }

  // Highest priority: real BLE disconnect event.
  if (bleDisconnectPending || visualMode == VISUAL_BLE_DISCONNECT) {
    if (visualMode != VISUAL_BLE_DISCONNECT) {
      beginVisualMode(VISUAL_BLE_DISCONNECT, true);
    }
    if (updateDisconnectVisual(now)) {
      bleDisconnectPending = false;
      beginVisualMode(computeBaseVisualMode(), true);
    }
    return;
  }

  // Startup animation only while truly cold / disconnected.
  if (startupPending && !targetValid && !bleConnected) {
    if (visualMode != VISUAL_STARTUP) {
      beginVisualMode(VISUAL_STARTUP, true);
    }
    if (updateStartupVisual(now)) {
      startupPending = false;
      beginVisualMode(computeBaseVisualMode(), true);
    }
    return;
  }

  // Next priority: BLE stop command acknowledgment.
  if (stopAnimPending || visualMode == VISUAL_STOP_ANIM) {
    if (visualMode != VISUAL_STOP_ANIM) {
      beginVisualMode(VISUAL_STOP_ANIM, true);
    }
    if (updateStopAnimVisual(now)) {
      stopAnimPending = false;
      beginVisualMode(computeBaseVisualMode(), true);
    }
    return;
  }

  // Next priority: BLE wake-up on real BLE STATE connect.
  if (bleWakePending || visualMode == VISUAL_BLE_WAKE) {
    if (visualMode != VISUAL_BLE_WAKE) {
      beginVisualMode(VISUAL_BLE_WAKE, true);
    }
    if (updateBleWakeVisual(now)) {
      bleWakePending = false;
      beginVisualMode(computeBaseVisualMode(), true);
    }
    return;
  }

  // Next priority: target-received acknowledgment.
  if (targetAckPending || visualMode == VISUAL_TARGET_ACK) {
    if (visualMode != VISUAL_TARGET_ACK) {
      beginVisualMode(VISUAL_TARGET_ACK, true);
    }
    if (updateTargetAckVisual(now)) {
      targetAckPending = false;
      beginVisualMode(computeBaseVisualMode(), true);
    }
    return;
  }

  // Next priority: cup-removal wipe.
  if (resetWipePending || visualMode == VISUAL_RESET_WIPE) {
    if (visualMode != VISUAL_RESET_WIPE) {
      beginVisualMode(VISUAL_RESET_WIPE, true);
    }
    if (updateResetWipeVisual(now)) {
      resetWipePending = false;
      beginVisualMode(computeBaseVisualMode(), true);
    }
    return;
  }

  // Trigger one-shot verdict transition animations when the verdict changes.
  LedState desiredVerdict = getDesiredVerdictState();
  if (desiredVerdict == LED_STATE_GREEN && lastAnimatedVerdict != LED_STATE_GREEN) {
    lastAnimatedVerdict = LED_STATE_GREEN;
    beginVisualMode(VISUAL_PASS_ANIM, true);
  } else if (desiredVerdict == LED_STATE_RED && lastAnimatedVerdict != LED_STATE_RED) {
    lastAnimatedVerdict = LED_STATE_RED;
    beginVisualMode(VISUAL_FAIL_ANIM, true);
  } else if (desiredVerdict == LED_STATE_OFF) {
    lastAnimatedVerdict = LED_STATE_OFF;
  }

  if (visualMode == VISUAL_PASS_ANIM) {
    if (updatePassAnimVisual(now)) {
      beginVisualMode(VISUAL_PASS_HOLD, true);
    }
    return;
  }

  if (visualMode == VISUAL_FAIL_ANIM) {
    if (updateFailAnimVisual(now)) {
      beginVisualMode(VISUAL_FAIL_HOLD, true);
    }
    return;
  }

  VisualMode baseMode = computeBaseVisualMode();
  if (visualMode != baseMode) {
    beginVisualMode(baseMode, true);
  }

  switch (visualMode) {
    case VISUAL_OFF:
      clearStripNoShow();
      showStrip();
      break;
    case VISUAL_IDLE_EMBER:
      updateIdleEmberVisual(now);
      break;
    case VISUAL_HEARTBEAT:
      updateHeartbeatVisual(now);
      break;
    case VISUAL_SCANNER:
      updateScannerVisual(now);
      break;
    case VISUAL_PASS_HOLD:
      updateSolidHoldVisual(colorGreen(255));
      break;
    case VISUAL_FAIL_HOLD:
      updateSolidHoldVisual(colorRed(255));
      break;
    default:
      break;
  }
}

// -------------------------
// Helpers
// -------------------------
bool isTargetLine(const char *line) {
  if (line[0] != 'T') return false;
  char c = line[1];
  return isdigit((unsigned char)c) || c == '+' || c == '-' || c == '.';
}

bool parseScaleLine(const char *line, bool &stableOut, float &weightOut) {
  if (!(strncmp(line, "ST,", 3) == 0 || strncmp(line, "US,", 3) == 0)) {
    return false;
  }

  stableOut = (strncmp(line, "ST,", 3) == 0);

  const char *comma = strchr(line, ',');
  if (!comma) return false;

  weightOut = atof(comma + 1);
  return true;
}

void clearDecisionWindow() {
  stableWindowCount = 0;
}

void clearPendingRunExitAnimations() {
  stopAnimPending = false;
  resetWipePending = false;
  bleDisconnectPending = false;
}

bool isCupRemovalReading() {
  return liveWeightGn < CUP_REMOVAL_THRESHOLD_GN;
}

bool isDuplicateTarget(float newTargetGn) {
  return targetValid && fabsf(newTargetGn - targetWeightGn) <= TARGET_DUPLICATE_EPSILON_GN;
}

bool shouldExpectScaleTraffic() {
  return bleConnected && targetValid && runVisualLatched && !decisionActive;
}

void clearRunStateOnly() {
  clearDecisionWindow();
  decisionActive = false;
  displayedWeightGn = 0.0f;
  cupRemovalCounter = 0;
  lastLoggedVerdictState = LED_STATE_OFF;
  lastAnimatedVerdict = LED_STATE_OFF;
  resetScannerMotionToCenter();
  runVisualLatched = false;
  runStartCounter = 0;
}

void pushStableSample(float w) {
  if (stableWindowCount < DECISION_SAMPLES) {
    stableWindow[stableWindowCount++] = w;
    return;
  }

  for (uint8_t i = 1; i < DECISION_SAMPLES; i++) {
    stableWindow[i - 1] = stableWindow[i];
  }
  stableWindow[DECISION_SAMPLES - 1] = w;
}

bool evaluateDecisionWindow(float &avgOut, float &spreadOut) {
  if (stableWindowCount < DECISION_SAMPLES) return false;

  float minW = stableWindow[0];
  float maxW = stableWindow[0];
  float sumW = 0.0f;

  for (uint8_t i = 0; i < DECISION_SAMPLES; i++) {
    float w = stableWindow[i];
    if (w < minW) minW = w;
    if (w > maxW) maxW = w;
    sumW += w;
  }

  avgOut = sumW / DECISION_SAMPLES;
  spreadOut = maxW - minW;
  return (spreadOut <= DECISION_MAX_SPREAD_GN);
}

void checkForRollingSettledDecision() {
  if (!targetValid) return;

  if (decisionActive && isCupRemovalReading()) {
    // Hold the last real verdict once the cup comes off. Low readings are for reset detection only.
    return;
  }

  if (!liveStable) {
    // Do not blank the visuals on momentary unstable lines; just wait for stable data again.
    return;
  }

  float errorGn = liveWeightGn - targetWeightGn;

  // Before monitoring is armed, stay below-target and keep the decision window clean.
  if (!decisionActive && errorGn < -ARM_BELOW_TARGET_GN) {
    clearDecisionWindow();
    return;
  }

  pushStableSample(liveWeightGn);

  float avgW = 0.0f;
  float spreadW = 0.0f;
  bool windowTightEnough = evaluateDecisionWindow(avgW, spreadW);

  Serial.print("WINDOW samples=");
  Serial.print(stableWindowCount);
  Serial.print(" avg=");
  Serial.print(avgW, 2);
  Serial.print(" spread=");
  Serial.println(spreadW, 3);

  if (!windowTightEnough) return;

  displayedWeightGn = avgW;

  if (!decisionActive) {
    decisionActive = true;
    Serial.print("SETTLED MONITOR ARMED @ ");
    Serial.print(displayedWeightGn, 2);
    Serial.println(" gn");
  } else {
    Serial.print("SETTLED UPDATE @ ");
    Serial.print(displayedWeightGn, 2);
    Serial.println(" gn");
  }

  float avgErrorGn = displayedWeightGn - targetWeightGn;
  Serial.print("ROLLING RESULT -> ");
  if (fabsf(avgErrorGn) <= FINAL_TOLERANCE_GN) {
    Serial.println("GREEN");
  } else {
    Serial.println("RED");
  }
}

void checkForCupRemovalReset() {
  if (!targetValid || !decisionActive) return;

  if (liveWeightGn < CUP_REMOVAL_THRESHOLD_GN) {
    if (cupRemovalCounter < CUP_REMOVAL_COUNT) cupRemovalCounter++;
  } else {
    cupRemovalCounter = 0;
  }

  Serial.print("CUP RESET counter=");
  Serial.print(cupRemovalCounter);
  Serial.print(" weight=");
  Serial.println(liveWeightGn, 2);

  if (cupRemovalCounter >= CUP_REMOVAL_COUNT) {
    LedState finalVerdict = getDesiredVerdictState();
    resetWipeColor = (finalVerdict == LED_STATE_GREEN) ? colorGreen(255) : colorRed(255);

    Serial.println("CUP REMOVED -> wipe to black / clear settled monitor / re-arm same target");

    clearRunStateOnly();
    resetWipePending = true;
  }
}

// -------------------------
// Line handlers
// -------------------------
void handleBleLine(const char *line) {
  if (line[0] == '\0') return;

  Serial.print("BLE: ");
  Serial.println(line);

  if (strcmp(line, "stop") == 0) {
    bool hadResult = decisionActive;
    stopAnimColor = getDisarmAnimColor();
    stopAnimStyle = hadResult ? STOP_ANIM_FULL_COLLAPSE : STOP_ANIM_CENTER_PINCH;
    Serial.print("BLE stop received -> ");
    if (stopAnimColor == colorGreen(255)) {
      Serial.println("green disarm collapse -> connected idle ember / off");
    } else if (hadResult) {
      Serial.println("red disarm collapse -> connected idle ember / off");
    } else {
      Serial.println("red center-pinch disarm -> connected idle ember / off");
    }
    targetWeightGn = 0.0f;
    targetValid = false;
    clearRunStateOnly();
    bleWakePending = false;
    targetAckPending = false;
    resetWipePending = false;
    stopAnimPending = true;
    return;
  }

  if (isTargetLine(line)) {
    float newTarget = atof(line + 1);
    if (newTarget > 0.0f) {
      if (isDuplicateTarget(newTarget)) {
        Serial.print("TARGET DUPLICATE IGNORED: ");
        Serial.println(newTarget, 3);
        return;
      }

      targetWeightGn = newTarget;
      targetValid = true;
      clearRunStateOnly();
      clearPendingRunExitAnimations();
      targetAckPending = true;

      Serial.print("TARGET CAPTURED: ");
      Serial.println(targetWeightGn, 3);
    }
  }
}

void handleScaleLine(const char *line) {
  if (line[0] == '\0') return;

  bool stable;
  float weight;

  if (!parseScaleLine(line, stable, weight)) {
    Serial.print("SCALE(raw): ");
    Serial.println(line);
    return;
  }

  liveStable = stable;
  liveWeightGn = weight;

  if (targetValid && !decisionActive && !runVisualLatched) {
    if (liveWeightGn >= RUN_START_THRESHOLD_GN) {
      if (runStartCounter < RUN_START_COUNT) runStartCounter++;
    } else {
      runStartCounter = 0;
    }

    if (runStartCounter >= RUN_START_COUNT) {
      runVisualLatched = true;
      Serial.println("RUN VISUAL -> SCANNER LATCHED");
    }
  }

  Serial.print("SCALE: ");
  Serial.print(stable ? "ST " : "US ");
  Serial.print(liveWeightGn, 2);
  Serial.println(" gn");

  if (targetValid) {
    float errorGn = liveWeightGn - targetWeightGn;
    Serial.print("ERROR: ");
    Serial.println(errorGn, 2);
  }

  checkForCupRemovalReset();
  if (resetWipePending) return;

  if (targetValid && decisionActive && isCupRemovalReading()) {
    logVerdictTransition();
    return;
  }

  checkForRollingSettledDecision();
  logVerdictTransition();
}

// -------------------------
// Buffer flushers
// -------------------------
void flushBleLine() {
  if (bleLen == 0) {
    bleOverflowed = false;
    return;
  }

  bleBuf[bleLen] = '\0';
  if (bleOverflowed) {
    Serial.println("WARN: BLE line truncated due to buffer overflow");
  }
  handleBleLine(bleBuf);
  bleLen = 0;
  bleOverflowed = false;
}

void flushScaleLine() {
  if (scaleLen == 0) {
    scaleOverflowed = false;
    return;
  }

  scaleBuf[scaleLen] = '\0';
  if (scaleOverflowed) {
    Serial.println("WARN: scale line truncated due to buffer overflow");
  }
  handleScaleLine(scaleBuf);
  scaleLen = 0;
  scaleOverflowed = false;
}

// -------------------------
// Readers
// -------------------------
void readBleChannel() {
  while (BLE_UART.available()) {
    int raw = BLE_UART.read();
    if (raw < 0) break;

    char c = (char)raw;
    lastBleByteTime = millis();
    bleTrafficSeen = true;
    bleStaleReported = false;

    if (c == '\r') continue;

    if (c == '\n') {
      flushBleLine();
      continue;
    }

    if (c >= 32 && c <= 126) {
      if (bleLen < sizeof(bleBuf) - 1) {
        bleBuf[bleLen++] = c;
      } else {
        bleOverflowed = true;
      }
    }
  }

  if (bleLen > 0 && (millis() - lastBleByteTime) > FRAME_GAP_MS) {
    flushBleLine();
  }
}

void readScaleChannel() {
  while (SCALE_UART.available()) {
    int raw = SCALE_UART.read();
    if (raw < 0) break;

    char c = (char)raw;
    lastScaleByteTime = millis();
    scaleTrafficSeen = true;
    scaleStaleReported = false;

    if (c == '\r') continue;

    if (c == '\n') {
      flushScaleLine();
      continue;
    }

    if (c >= 32 && c <= 126) {
      if (scaleLen < sizeof(scaleBuf) - 1) {
        scaleBuf[scaleLen++] = c;
      } else {
        scaleOverflowed = true;
      }
    }
  }

  if (scaleLen > 0 && (millis() - lastScaleByteTime) > FRAME_GAP_MS) {
    flushScaleLine();
  }
}

void updateChannelHealth() {
  uint32_t now = millis();

  if (ENABLE_BLE_STALE_WARNINGS && bleConnected && bleTrafficSeen && !bleStaleReported && (now - lastBleByteTime) > BLE_STALE_TIMEOUT_MS) {
    bleStaleReported = true;
    Serial.println("WARN: BLE sniff channel stale");
  }

  if (!shouldExpectScaleTraffic()) {
    scaleStaleReported = false;
    return;
  }

  if (ENABLE_SCALE_STALE_WARNINGS && scaleTrafficSeen && !scaleStaleReported && (now - lastScaleByteTime) > SCALE_STALE_TIMEOUT_MS) {
    scaleStaleReported = true;
    Serial.println("WARN: scale sniff channel stale during active run");
  }
}

// -------------------------
// BLE STATE handling
// -------------------------
void onStableStateChange(bool newState) {
  bleConnected = newState;

  Serial.print("BLE STATE -> ");
  Serial.println(bleConnected ? "CONNECTED" : "DISCONNECTED");

  startupPending = false;

  if (bleConnected) {
    bleWakePending = true;
    bleDisconnectPending = false;
  } else {
    targetWeightGn = 0.0f;
    targetValid = false;
    clearRunStateOnly();
    targetAckPending = false;
    stopAnimPending = false;
    resetWipePending = false;
    bleWakePending = false;
    bleDisconnectPending = true;
  }
}

void updateBleStateInput() {
  bool rawNow = digitalRead(BLE_STATE_PIN);

  if (rawNow != stateRaw) {
    stateRaw = rawNow;
    stateCandidate = rawNow;
    stateCandidateSinceMs = millis();

    Serial.print("BLE STATE raw -> ");
    Serial.println(rawNow ? "HIGH" : "LOW");
  }

  if (stateCandidate != bleConnected) {
    if ((millis() - stateCandidateSinceMs) >= STATE_DEBOUNCE_MS) {
      onStableStateChange(stateCandidate);
    }
  }
}

// -------------------------
// Setup / loop
// -------------------------
void setup() {
  stopAnimColor = colorRed(255);
  Serial.begin(USB_BAUD);
  delay(250);

  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.show();
  clearStripNoShow();
  showStrip();
  resetScannerMotionToCenter();

  BLE_UART.begin(BLE_BAUD);
  SCALE_UART.begin(SCALE_BAUD, SERIAL_8N1, SCALE_RX_PIN, -1);

  pinMode(BLE_STATE_PIN, INPUT_MODE);

  Serial.println();
  Serial.println("PowderBot 9000 - working strip sketch with personality");
  Serial.println("-----------------------------------------------------");
  stateRaw = digitalRead(BLE_STATE_PIN);
  stateCandidate = stateRaw;
  bleConnected = stateRaw;

  Serial.println("GPIO21 = BLE target sniff");
  Serial.println("GPIO22 = BLE STATE (LOW=disconnected, HIGH=connected)");
  Serial.println("GPIO35 = scale stream sniff");
  Serial.println("GPIO17 = LED strip data");
  Serial.println("BLE wake            = blue wake-up pulse on real connect");
  Serial.println("BLE disconnect      = blue collapse on real disconnect");
  Serial.println("BLE ready           = soft red center ember while connected");
  Serial.println("Idle / waiting      = stronger red heartbeat once target is set");
  Serial.println("Target received     = red center-out acknowledgment");
  Serial.println("Throw in progress   = red scanner once run is latched");
  Serial.println("PASS                = green bloom then solid green");
  Serial.println("FAIL                = red pulse then solid red");
  Serial.println("Cup removed         = wipe to black, then re-arm same target");
  Serial.println("BLE stop / disarm   = truth-color collapse; no-result uses red center pinch");
  Serial.print("Initial BLE STATE: ");
  Serial.println(bleConnected ? "CONNECTED" : "DISCONNECTED");
  Serial.print("Pixels: ");
  Serial.println(NUM_PIXELS);
  Serial.print("Brightness: ");
  Serial.println(LED_BRIGHTNESS);
  Serial.print("Tolerance: +/-");
  Serial.print(FINAL_TOLERANCE_GN, 2);
  Serial.println(" gn");
  Serial.print("Arm below target: ");
  Serial.print(ARM_BELOW_TARGET_GN, 2);
  Serial.println(" gn");
  Serial.print("Decision samples: ");
  Serial.println(DECISION_SAMPLES);
  Serial.print("Decision max spread: ");
  Serial.print(DECISION_MAX_SPREAD_GN, 3);
  Serial.println(" gn");
  Serial.print("Run-start threshold: ");
  Serial.print(RUN_START_THRESHOLD_GN, 2);
  Serial.println(" gn");
  Serial.print("Run-start count: ");
  Serial.println(RUN_START_COUNT);
  Serial.print("Cup removal threshold: ");
  Serial.print(CUP_REMOVAL_THRESHOLD_GN, 2);
  Serial.println(" gn");
  Serial.print("Cup removal count: ");
  Serial.println(CUP_REMOVAL_COUNT);
  Serial.print("Duplicate target epsilon: ");
  Serial.print(TARGET_DUPLICATE_EPSILON_GN, 4);
  Serial.println(" gn");
  Serial.print("BLE stale warnings: ");
  Serial.println(ENABLE_BLE_STALE_WARNINGS ? "ON" : "OFF");
  Serial.print("Scale stale warnings: ");
  Serial.println(ENABLE_SCALE_STALE_WARNINGS ? "ON" : "OFF");
  Serial.println();
}

void loop() {
  updateBleStateInput();
  readBleChannel();
  readScaleChannel();
  updateChannelHealth();
  updateVisuals();
}
