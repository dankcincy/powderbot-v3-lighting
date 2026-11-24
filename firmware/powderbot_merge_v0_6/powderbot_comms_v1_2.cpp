#include "powderbot_comms_v1_2.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

namespace {

static const PbCommsHubConfig kDefaultConfig = {
  .bleName = "BT05",
  .serviceUuid = "0000FFE0-0000-1000-8000-00805F9B34FB",
  .charUuid = "0000FFE1-0000-1000-8000-00805F9B34FB",
  .v3RxPin = 34,
  .v3TxPin = 22,
  .v3Baud = 9600,
  .v3FrameMax = 160,
  .v3IdleFlushMs = 30,
  .advertisingRestartDelayMs = 120,
  .trafficTapEnabled = true,
  .tapAppToV3 = true,
  .tapV3ToApp = true,
  .tapScaleMain = true,
  .tapScaleCmd = true,
  .tapScaleSummary = true,
  .tapFrameGapMs = 20,
  .tapBufMax = 192,
  .scaleLinkMode = PB_SCALE_LINK_PASSIVE_ONLY,
  .scaleRxPin = 32,
  .scaleTxPin = -1,
  .scaleCmdTapRxPin = 33,
  .scaleBaud = 19200,
  .scaleSerialConfig = SERIAL_8N1,
  .scaleCmdTerminator = "\r\n",
  .dropV3TrafficWhenBleDisconnected = true,
};

const PbCommsHubConfig* g_cfg = nullptr;
bool g_started = false;

BLEServer* g_server = nullptr;
BLEAdvertising* g_advertising = nullptr;
BLECharacteristic* g_characteristic = nullptr;
volatile bool g_restartAdvertisingRequested = false;
volatile bool g_bleClientConnected = false;
uint32_t g_restartAdvertisingAtMs = 0;

HardwareSerial g_v3Uart(2);
HardwareSerial g_scaleLinkUart(1);
SoftwareSerial* g_scaleCmdTapUart = nullptr;

PbScaleLinkState g_scaleState = {};
PbCommsHubStats g_stats = {};
PbCommsEventHandlers g_handlers = {};

enum PbQueuedEventType : uint8_t {
  PB_QUEUED_EVT_BLE_STATE = 0,
  PB_QUEUED_EVT_COMMAND,
  PB_QUEUED_EVT_RAW_TRAFFIC,
};

struct PbQueuedEvent {
  PbQueuedEventType type = PB_QUEUED_EVT_BLE_STATE;
  bool connected = false;
  PowderBotProtocol::PbCommand command{};
  char rawChannel[PowderBotProtocol::PB_TEXT_SMALL_LEN] = {0};
  char rawLine[PowderBotProtocol::PB_TEXT_LARGE_LEN] = {0};
};

static constexpr uint8_t kEventQueueCapacity = 32;
static constexpr uint8_t kEventQueueReserveForControl = 6;
PbQueuedEvent g_eventQueue[kEventQueueCapacity];
volatile uint8_t g_eventQueueHead = 0;
volatile uint8_t g_eventQueueTail = 0;
volatile uint32_t g_eventQueueDrops = 0;

char* g_v3FrameBuf = nullptr;
size_t g_v3FrameLen = 0;
bool g_v3FrameOverflowed = false;
uint32_t g_lastV3ByteMs = 0;

char* g_scaleMainBuf = nullptr;
size_t g_scaleMainLen = 0;
bool g_scaleMainOverflowed = false;
uint32_t g_lastScaleMainByteMs = 0;

char* g_scaleCmdBuf = nullptr;
size_t g_scaleCmdLen = 0;
bool g_scaleCmdOverflowed = false;
uint32_t g_lastScaleCmdByteMs = 0;

void logLine(const char* text) {
  Serial.println(text);
}

void copyBoundedString(char* dst, size_t dstLen, const char* src) {
  if (!dst || dstLen == 0) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, dstLen - 1);
  dst[dstLen - 1] = '\0';
}

uint8_t queueNextIndex(uint8_t index) {
  return static_cast<uint8_t>((index + 1) % kEventQueueCapacity);
}

uint8_t queueUsage() {
  return static_cast<uint8_t>((g_eventQueueHead + kEventQueueCapacity - g_eventQueueTail) % kEventQueueCapacity);
}

bool queueIsFull() {
  return queueNextIndex(g_eventQueueHead) == g_eventQueueTail;
}

void dropOldestQueuedEvent() {
  if (g_eventQueueTail == g_eventQueueHead) return;
  g_eventQueueTail = queueNextIndex(g_eventQueueTail);
  ++g_eventQueueDrops;
}

void preserveControlQueueHeadroom(const PbQueuedEvent& incoming) {
  if (incoming.type == PB_QUEUED_EVT_RAW_TRAFFIC) return;
  while (queueIsFull() && g_eventQueueTail != g_eventQueueHead &&
         g_eventQueue[g_eventQueueTail].type == PB_QUEUED_EVT_RAW_TRAFFIC) {
    dropOldestQueuedEvent();
  }
}

bool enqueueEvent(const PbQueuedEvent& event) {
  if (event.type == PB_QUEUED_EVT_RAW_TRAFFIC &&
      queueUsage() >= static_cast<uint8_t>(kEventQueueCapacity - kEventQueueReserveForControl)) {
    ++g_eventQueueDrops;
    return false;
  }

  preserveControlQueueHeadroom(event);

  if (queueIsFull()) {
    ++g_eventQueueDrops;
    return false;
  }

  const uint8_t nextHead = queueNextIndex(g_eventQueueHead);
  g_eventQueue[g_eventQueueHead] = event;
  g_eventQueueHead = nextHead;
  return true;
}

bool dequeueEvent(PbQueuedEvent& eventOut) {
  if (g_eventQueueTail == g_eventQueueHead) return false;
  eventOut = g_eventQueue[g_eventQueueTail];
  g_eventQueueTail = queueNextIndex(g_eventQueueTail);
  return true;
}

void dispatchQueuedEvents() {
  PbQueuedEvent event;
  while (dequeueEvent(event)) {
    switch (event.type) {
      case PB_QUEUED_EVT_BLE_STATE:
        if (g_handlers.onBleConnectionChanged) {
          g_handlers.onBleConnectionChanged(event.connected);
        }
        break;

      case PB_QUEUED_EVT_COMMAND:
        if (g_handlers.onCommandReceived) {
          g_handlers.onCommandReceived(event.command);
        }
        break;

      case PB_QUEUED_EVT_RAW_TRAFFIC:
        if (g_handlers.onRawTrafficLine) {
          g_handlers.onRawTrafficLine(event.rawChannel, event.rawLine);
        }
        break;
    }
  }

  if (g_eventQueueDrops != 0 && g_cfg && g_cfg->trafficTapEnabled) {
    Serial.print("WARN: comms event queue dropped ");
    Serial.print(g_eventQueueDrops);
    Serial.println(" event(s)");
    g_eventQueueDrops = 0;
  }
}

void emitRawTrafficLine(const char* channel, const char* line) {
  if (!g_handlers.onRawTrafficLine || !channel || !line) return;

  PbQueuedEvent event{};
  event.type = PB_QUEUED_EVT_RAW_TRAFFIC;
  copyBoundedString(event.rawChannel, sizeof(event.rawChannel), channel);
  copyBoundedString(event.rawLine, sizeof(event.rawLine), line);
  enqueueEvent(event);
}

void trimInPlace(char* s) {
  if (!s) return;
  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == '\r' || s[len - 1] == '\n' || isspace(static_cast<unsigned char>(s[len - 1])))) {
    s[--len] = '\0';
  }

  size_t start = 0;
  while (s[start] && isspace(static_cast<unsigned char>(s[start]))) {
    ++start;
  }
  if (start > 0) {
    memmove(s, s + start, strlen(s + start) + 1);
  }
}

bool parseLegacyCommandText(const uint8_t* data, size_t len, PowderBotProtocol::PbCommand& out) {
  if (!data || len == 0) return false;

  char work[PowderBotProtocol::PB_LEGACY_TEXT_LEN] = {0};
  const size_t copyLen = (len < sizeof(work) - 1) ? len : (sizeof(work) - 1);
  memcpy(work, data, copyLen);
  work[copyLen] = '\0';
  trimInPlace(work);
  if (!work[0]) return false;

  return PowderBotProtocol::pbMakeLegacyCommandFromText(work, out);
}

void printEscapedBytes(const char* label, const uint8_t* data, size_t len) {
  if (!g_cfg || !g_cfg->trafficTapEnabled) return;

  Serial.print(label);
  for (size_t i = 0; i < len; ++i) {
    const uint8_t b = data[i];
    switch (b) {
      case '\r': Serial.print("\\r"); break;
      case '\n': Serial.print("\\n"); break;
      case '\t': Serial.print("\\t"); break;
      default:
        if (b >= 32 && b <= 126) {
          Serial.write((char)b);
        } else {
          char tmp[5];
          snprintf(tmp, sizeof(tmp), "\\x%02X", b);
          Serial.print(tmp);
        }
        break;
    }
  }
  Serial.println();
}

void resetFrame(char* buf, size_t& len, bool& overflowed) {
  if (buf) buf[0] = '\0';
  len = 0;
  overflowed = false;
}

bool appendPrintableOrControl(char* buf, size_t cap, size_t& len, bool& overflowed, char c) {
  if (c == '\r' || c == '\n' || (c >= 32 && c <= 126)) {
    if (len < cap - 1) {
      buf[len++] = c;
      buf[len] = '\0';
      return true;
    }
    overflowed = true;
  }
  return false;
}

PowderBotProtocol::PbScaleSample parseScaleSample(const char* line) {
  PowderBotProtocol::PbScaleSample sample{};
  if (!line) return sample;

  while (*line == ' ') ++line;
  if (strncmp(line, "ST,", 3) != 0 && strncmp(line, "US,", 3) != 0) {
    return sample;
  }

  sample.stable = (line[0] == 'S');
  const char* p = line + 3;
  while (*p == ' ') ++p;

  char numberBuf[24];
  size_t idx = 0;
  while (*p && idx < sizeof(numberBuf) - 1) {
    if ((*p >= '0' && *p <= '9') || *p == '+' || *p == '-' || *p == '.') {
      numberBuf[idx++] = *p++;
    } else {
      break;
    }
  }
  numberBuf[idx] = '\0';
  if (idx == 0) return sample;
  if (!strstr(line, "GN")) return sample;

  sample.weightGn = strtof(numberBuf, nullptr);
  sample.valid = true;
  return sample;
}

void logScaleSummary(const PowderBotProtocol::PbScaleSample& sample) {
  if (!g_cfg || !g_cfg->trafficTapEnabled || !g_cfg->tapScaleSummary || !sample.valid) {
    return;
  }
  Serial.print("SCALE PARSED: ");
  Serial.print(sample.stable ? "stable" : "unstable");
  Serial.print(", ");
  Serial.print(sample.weightGn, 2);
  Serial.println(" gn");
}

void noteScaleLine(const char* line) {
  const PowderBotProtocol::PbScaleSample sample = parseScaleSample(line);
  if (!sample.valid) return;
  g_scaleState.lastSampleValid = true;
  g_scaleState.lastSample = sample;
  g_scaleState.streamLikelyActive = true;
  g_scaleState.lastRxMs = millis();
  logScaleSummary(sample);
  if (g_handlers.onScaleSampleReceived) {
    g_handlers.onScaleSampleReceived(sample);
  }
}

void rememberScaleCommand(const char* cmd) {
  if (!cmd) return;
  strncpy(g_scaleState.lastCommand, cmd, sizeof(g_scaleState.lastCommand) - 1);
  g_scaleState.lastCommand[sizeof(g_scaleState.lastCommand) - 1] = '\0';
  g_scaleState.lastCmdMs = millis();
  if (strcmp(cmd, "SIR") == 0) {
    g_scaleState.streamRequested = true;
  } else if (strcmp(cmd, "C") == 0) {
    g_scaleState.streamRequested = false;
    g_scaleState.streamLikelyActive = false;
  }
}

bool scaleLinkCanTransmit() {
  return g_scaleState.linkInitialized && g_scaleState.activeModeEnabled;
}

bool v3FrameBufferReady() {
  return g_v3FrameBuf != nullptr && g_cfg && g_cfg->v3FrameMax > 0;
}

bool scaleMainBufferReady() {
  return g_scaleMainBuf != nullptr && g_cfg && g_cfg->tapBufMax > 0;
}

bool scaleCmdBufferReady() {
  return g_scaleCmdBuf != nullptr && g_cfg && g_cfg->tapBufMax > 0;
}

void flushV3Frame(bool forceFlush) {
  if (!v3FrameBufferReady() || g_v3FrameLen == 0) return;

  if (!forceFlush && (millis() - g_lastV3ByteMs) < g_cfg->v3IdleFlushMs) {
    return;
  }

  if (g_v3FrameOverflowed && g_cfg->trafficTapEnabled) {
    logLine("WARN: V3->APP frame truncated due to buffer overflow");
  }

  if (g_cfg->trafficTapEnabled && g_cfg->tapV3ToApp) {
    printEscapedBytes("V3->APP: ", reinterpret_cast<const uint8_t*>(g_v3FrameBuf), g_v3FrameLen);
  }
  emitRawTrafficLine("V3_FRAME", g_v3FrameBuf);

  if (g_characteristic && g_bleClientConnected) {
    g_characteristic->setValue(reinterpret_cast<uint8_t*>(g_v3FrameBuf), g_v3FrameLen);
    g_characteristic->notify();
    ++g_stats.v3FramesForwarded;
  } else if (g_cfg->dropV3TrafficWhenBleDisconnected) {
    ++g_stats.v3FramesDroppedWhileDisconnected;
  }

  resetFrame(g_v3FrameBuf, g_v3FrameLen, g_v3FrameOverflowed);
}

void restartAdvertisingCleanly() {
  if (!g_advertising) return;
  BLEDevice::stopAdvertising();
  BLEDevice::startAdvertising();
  if (g_cfg->trafficTapEnabled) {
    logLine("BLE: advertising restarted");
  }
}

void initScaleLink() {
  g_scaleState = {};
  if (g_cfg->scaleLinkMode == PB_SCALE_LINK_DISABLED) {
    return;
  }

  const bool needPassiveScaleRx =
      (g_cfg->scaleLinkMode == PB_SCALE_LINK_PASSIVE_ONLY) &&
      (g_cfg->scaleRxPin >= 0) &&
      (g_cfg->tapScaleMain || g_cfg->tapScaleSummary || (g_handlers.onScaleSampleReceived != nullptr));

  if (needPassiveScaleRx) {
    g_scaleLinkUart.begin(g_cfg->scaleBaud, g_cfg->scaleSerialConfig, g_cfg->scaleRxPin, -1);
    g_scaleState.linkInitialized = true;
    logLine("SCALE LINK: passive main-stream RX enabled");
  } else if (g_cfg->scaleLinkMode == PB_SCALE_LINK_ACTIVE_RS232) {
    if (g_cfg->scaleTxPin < 0) {
      logLine("WARN: active scale mode selected but scaleTxPin is not set");
    } else {
      g_scaleLinkUart.begin(g_cfg->scaleBaud, g_cfg->scaleSerialConfig, g_cfg->scaleRxPin, g_cfg->scaleTxPin);
      g_scaleState.linkInitialized = true;
      g_scaleState.activeModeEnabled = true;
      logLine("SCALE LINK: active RS232 mode enabled");
    }
  }

  if (g_cfg->tapScaleCmd && g_cfg->scaleCmdTapRxPin >= 0) {
    if (g_scaleCmdTapUart) {
      delete g_scaleCmdTapUart;
      g_scaleCmdTapUart = nullptr;
    }
    g_scaleCmdTapUart = new SoftwareSerial(g_cfg->scaleCmdTapRxPin, -1, false);
    if (!g_scaleCmdTapUart) {
      logLine("WARN: failed to allocate passive command-side tap serial instance");
    } else {
      g_scaleCmdTapUart->begin(g_cfg->scaleBaud);
      logLine("SCALE LINK: passive command-side tap enabled");
    }
  }
}

class ServerCallbacks : public BLEServerCallbacks {
 public:
  void onConnect(BLEServer* server) override {
    (void)server;
    g_bleClientConnected = true;
    if (g_cfg->trafficTapEnabled) {
      logLine("BLE STATE: connected");
    }
    PbQueuedEvent event{};
    event.type = PB_QUEUED_EVT_BLE_STATE;
    event.connected = true;
    enqueueEvent(event);
  }

  void onDisconnect(BLEServer* server) override {
    (void)server;
    g_bleClientConnected = false;
    g_restartAdvertisingRequested = true;
    g_restartAdvertisingAtMs = millis() + g_cfg->advertisingRestartDelayMs;
    if (g_cfg->trafficTapEnabled) {
      logLine("BLE STATE: disconnected");
    }
    if (g_cfg->dropV3TrafficWhenBleDisconnected) {
      resetFrame(g_v3FrameBuf, g_v3FrameLen, g_v3FrameOverflowed);
    }
    PbQueuedEvent event{};
    event.type = PB_QUEUED_EVT_BLE_STATE;
    event.connected = false;
    enqueueEvent(event);
  }
};

class CharCallbacks : public BLECharacteristicCallbacks {
 public:
  void onWrite(BLECharacteristic* c) override {
    const auto value = c->getValue();
    const size_t valueLen = value.length();
    if (valueLen == 0) return;

    const uint8_t* valueBytes = reinterpret_cast<const uint8_t*>(value.c_str());

    if (g_cfg->trafficTapEnabled && g_cfg->tapAppToV3) {
      printEscapedBytes("APP->V3: ", valueBytes, valueLen);
    }

    PowderBotProtocol::PbCommand command;
    if (parseLegacyCommandText(valueBytes, valueLen, command)) {
      emitRawTrafficLine("APP_CMD", command.text);
      PbQueuedEvent event{};
      event.type = PB_QUEUED_EVT_COMMAND;
      event.command = command;
      enqueueEvent(event);
    }

    g_v3Uart.write(valueBytes, valueLen);
    ++g_stats.bleWritesForwarded;
  }
};

void initBle() {
  BLEDevice::init(g_cfg->bleName);
  g_server = BLEDevice::createServer();
  g_server->setCallbacks(new ServerCallbacks());

  BLEService* service = g_server->createService(BLEUUID(g_cfg->serviceUuid));
  g_characteristic = service->createCharacteristic(
      BLEUUID(g_cfg->charUuid),
      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_WRITE_NR |
      BLECharacteristic::PROPERTY_NOTIFY);
  g_characteristic->addDescriptor(new BLE2902());
  g_characteristic->setCallbacks(new CharCallbacks());
  g_characteristic->setValue("");
  service->start();

  g_advertising = BLEDevice::getAdvertising();
  g_advertising->addServiceUUID(BLEUUID(g_cfg->serviceUuid));
  g_advertising->setScanResponse(false);
  g_advertising->setMinPreferred(0x06);
  g_advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void handleScaleMainLine(char* line) {
  ++g_stats.scaleMainLinesSeen;
  emitRawTrafficLine("SCALE_MAIN", line);
  if (g_cfg->trafficTapEnabled && g_cfg->tapScaleMain) {
    Serial.print("SCALE: ");
    Serial.println(line);
  }
  noteScaleLine(line);
}

void handleScaleCmdLine(char* line) {
  ++g_stats.scaleCmdLinesSeen;
  emitRawTrafficLine("SCALE_CMD", line);
  if (g_cfg->trafficTapEnabled && g_cfg->tapScaleCmd) {
    Serial.print("SCALE CMD: ");
    Serial.println(line);
  }
  rememberScaleCommand(line);
}

void readScaleMain() {
  if (!g_scaleState.linkInitialized || !scaleMainBufferReady()) return;

  while (g_scaleLinkUart.available() > 0) {
    const int raw = g_scaleLinkUart.read();
    if (raw < 0) break;
    const char c = (char)raw;
    g_lastScaleMainByteMs = millis();

    if (c == '\r') continue;
    if (c == '\n') {
      if (g_scaleMainLen > 0) {
        handleScaleMainLine(g_scaleMainBuf);
      }
      resetFrame(g_scaleMainBuf, g_scaleMainLen, g_scaleMainOverflowed);
      continue;
    }
    appendPrintableOrControl(g_scaleMainBuf, g_cfg->tapBufMax, g_scaleMainLen, g_scaleMainOverflowed, c);
  }

  if (g_scaleMainLen > 0 && (millis() - g_lastScaleMainByteMs) > g_cfg->tapFrameGapMs) {
    if (g_scaleMainOverflowed && g_cfg->trafficTapEnabled) {
      logLine("WARN: SCALE line truncated due to frame-gap flush");
    }
    handleScaleMainLine(g_scaleMainBuf);
    resetFrame(g_scaleMainBuf, g_scaleMainLen, g_scaleMainOverflowed);
  }
}

void readScaleCmdTap() {
  if (!g_scaleCmdTapUart || !scaleCmdBufferReady()) return;

  while (g_scaleCmdTapUart->available() > 0) {
    const int raw = g_scaleCmdTapUart->read();
    if (raw < 0) break;
    const char c = (char)raw;
    g_lastScaleCmdByteMs = millis();

    if (c == '\r') continue;
    if (c == '\n') {
      if (g_scaleCmdLen > 0) {
        handleScaleCmdLine(g_scaleCmdBuf);
      }
      resetFrame(g_scaleCmdBuf, g_scaleCmdLen, g_scaleCmdOverflowed);
      continue;
    }
    appendPrintableOrControl(g_scaleCmdBuf, g_cfg->tapBufMax, g_scaleCmdLen, g_scaleCmdOverflowed, c);
  }

  if (g_scaleCmdLen > 0 && (millis() - g_lastScaleCmdByteMs) > g_cfg->tapFrameGapMs) {
    if (g_scaleCmdOverflowed && g_cfg->trafficTapEnabled) {
      logLine("WARN: SCALE CMD line truncated due to frame-gap flush");
    }
    handleScaleCmdLine(g_scaleCmdBuf);
    resetFrame(g_scaleCmdBuf, g_scaleCmdLen, g_scaleCmdOverflowed);
  }
}

void pollScaleSessionHealth() {
  if (!g_scaleState.streamRequested) return;
  const uint32_t nowMs = millis();
  if (g_scaleState.lastRxMs != 0 && (nowMs - g_scaleState.lastRxMs) > 1000 && g_scaleState.streamLikelyActive) {
    g_scaleState.streamLikelyActive = false;
    if (g_cfg->trafficTapEnabled) {
      logLine("SCALE SESSION: stream appears stale/inactive");
    }
  }
}


}  // namespace

void pbCommsSetEventHandlers(const PbCommsEventHandlers& handlers) {
  g_handlers = handlers;
}

const PbCommsHubConfig& pbCommsDefaultConfig() {
  return kDefaultConfig;
}

void pbCommsBegin(const PbCommsHubConfig& config) {
  g_cfg = &config;
  g_started = true;
  g_stats = {};
  g_scaleState = {};
  g_bleClientConnected = false;
  g_restartAdvertisingRequested = false;

  if (g_v3FrameBuf) free(g_v3FrameBuf);
  if (g_scaleMainBuf) free(g_scaleMainBuf);
  if (g_scaleCmdBuf) free(g_scaleCmdBuf);
  g_v3FrameBuf = (char*)calloc(g_cfg->v3FrameMax + 1, 1);
  g_scaleMainBuf = (char*)calloc(g_cfg->tapBufMax + 1, 1);
  g_scaleCmdBuf = (char*)calloc(g_cfg->tapBufMax + 1, 1);

  if (!g_v3FrameBuf) {
    logLine("WARN: V3 frame buffer allocation failed; V3 framing disabled");
  }
  if (!g_scaleMainBuf) {
    logLine("WARN: scale main tap buffer allocation failed; scale RX framing disabled");
  }
  if (!g_scaleCmdBuf) {
    logLine("WARN: scale command tap buffer allocation failed; command tap framing disabled");
  }

  g_eventQueueHead = 0;
  g_eventQueueTail = 0;
  g_eventQueueDrops = 0;

  g_v3Uart.begin(g_cfg->v3Baud, SERIAL_8N1, g_cfg->v3RxPin, g_cfg->v3TxPin);
  initScaleLink();
  initBle();
  pbCommsPrintBanner();

}

void pbCommsUpdate() {
  if (!g_started || !g_cfg) return;

  if (g_restartAdvertisingRequested && static_cast<int32_t>(millis() - g_restartAdvertisingAtMs) >= 0) {
    g_restartAdvertisingRequested = false;
    restartAdvertisingCleanly();
  }

  while (v3FrameBufferReady() && g_v3Uart.available() > 0) {
    const int raw = g_v3Uart.read();
    if (raw < 0) break;
    const char ch = (char)raw;
    g_lastV3ByteMs = millis();
    appendPrintableOrControl(g_v3FrameBuf, g_cfg->v3FrameMax + 1, g_v3FrameLen, g_v3FrameOverflowed, ch);
    if (g_v3FrameLen >= g_cfg->v3FrameMax || ch == '\n') {
      flushV3Frame(true);
    }
  }

  flushV3Frame(false);
  readScaleMain();
  readScaleCmdTap();
  pollScaleSessionHealth();
  dispatchQueuedEvents();
}

bool pbCommsBleConnected() {
  return g_bleClientConnected;
}

const PbScaleLinkState& pbCommsScaleState() {
  return g_scaleState;
}

const PbCommsHubStats& pbCommsStats() {
  return g_stats;
}

bool pbCommsScaleSendRaw(const char* cmd) {
  if (!g_started || !g_cfg || !scaleLinkCanTransmit() || !cmd || !*cmd) {
    return false;
  }
  g_scaleLinkUart.print(cmd);
  g_scaleLinkUart.print(g_cfg->scaleCmdTerminator);
  rememberScaleCommand(cmd);
  if (g_cfg->trafficTapEnabled) {
    Serial.print("HUB->SCALE: ");
    Serial.print(cmd);
    Serial.print(" terminator=");
    printEscapedBytes("", reinterpret_cast<const uint8_t*>(g_cfg->scaleCmdTerminator), strlen(g_cfg->scaleCmdTerminator));
  }
  return true;
}

bool pbCommsScaleCancelContinuous() { return pbCommsScaleSendRaw("C"); }
bool pbCommsScaleRequestImmediate() { return pbCommsScaleSendRaw("Q"); }
bool pbCommsScaleRequestStable() { return pbCommsScaleSendRaw("S"); }
bool pbCommsScaleRequestImmediateAlt() { return pbCommsScaleSendRaw("SI"); }
bool pbCommsScaleStartContinuous() { return pbCommsScaleSendRaw("SIR"); }

void pbCommsPrintBanner(Stream& out) {
  out.println();
  out.println("powderbot_comms_v1_2");
  out.println("- Raw BT05-compatible comms module with future-ready system architecture");
  out.println("- Traffic tap, FX scale-link scaffolding, and clean merge-ready comms foundation");
}

void pbCommsPrintState(Stream& out) {
  out.print("BLE: ");
  out.println(g_bleClientConnected ? "connected" : "disconnected");
  out.print("Scale link initialized: ");
  out.println(g_scaleState.linkInitialized ? "yes" : "no");
  out.print("Scale active mode: ");
  out.println(g_scaleState.activeModeEnabled ? "yes" : "no");
  out.print("Scale stream requested: ");
  out.println(g_scaleState.streamRequested ? "yes" : "no");
  out.print("Scale stream likely active: ");
  out.println(g_scaleState.streamLikelyActive ? "yes" : "no");
  out.print("Last scale command: ");
  out.println(g_scaleState.lastCommand[0] ? g_scaleState.lastCommand : "<none>");
  out.print("V3 frames forwarded: ");
  out.println(g_stats.v3FramesForwarded);
  out.print("V3 frames dropped while BLE disconnected: ");
  out.println(g_stats.v3FramesDroppedWhileDisconnected);
  out.print("Queued event drops: ");
  out.println(g_eventQueueDrops);
}

bool pbCommsHandleConsoleCommand(const char* line, Stream& out) {
  if (!line || !*line) return false;

  if (strcmp(line, "scale c") == 0) return pbCommsScaleCancelContinuous();
  if (strcmp(line, "scale q") == 0) return pbCommsScaleRequestImmediate();
  if (strcmp(line, "scale s") == 0) return pbCommsScaleRequestStable();
  if (strcmp(line, "scale si") == 0) return pbCommsScaleRequestImmediateAlt();
  if (strcmp(line, "scale sir") == 0) return pbCommsScaleStartContinuous();
  if (strcmp(line, "scale state") == 0) {
    pbCommsPrintState(out);
    return true;
  }

  return false;
}

