#pragma once

#include <Arduino.h>
#include <stdint.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <HardwareSerial.h>
#include <SoftwareSerial.h>

#include "powderbot_comms_protocol.h"

enum PbScaleLinkMode : uint8_t {
  PB_SCALE_LINK_DISABLED = 0,
  PB_SCALE_LINK_PASSIVE_ONLY,
  PB_SCALE_LINK_ACTIVE_RS232
};

struct PbCommsHubConfig {
  const char* bleName;
  const char* serviceUuid;
  const char* charUuid;

  int v3RxPin;
  int v3TxPin;
  uint32_t v3Baud;
  size_t v3FrameMax;
  uint32_t v3IdleFlushMs;
  uint32_t advertisingRestartDelayMs;

  bool trafficTapEnabled;
  bool tapAppToV3;
  bool tapV3ToApp;
  bool tapScaleMain;
  bool tapScaleCmd;
  bool tapScaleSummary;
  uint32_t tapFrameGapMs;
  size_t tapBufMax;

  PbScaleLinkMode scaleLinkMode;
  int scaleRxPin;
  int scaleTxPin;
  int scaleCmdTapRxPin;
  uint32_t scaleBaud;
  uint32_t scaleSerialConfig;
  const char* scaleCmdTerminator;
  bool dropV3TrafficWhenBleDisconnected;
};

struct PbScaleLinkState {
  bool linkInitialized;
  bool activeModeEnabled;
  bool streamRequested;
  bool streamLikelyActive;
  bool lastSampleValid;
  PowderBotProtocol::PbScaleSample lastSample;
  uint32_t lastRxMs;
  uint32_t lastCmdMs;
  char lastCommand[16];
};

struct PbCommsHubStats {
  uint32_t v3FramesForwarded;
  uint32_t v3FramesDroppedWhileDisconnected;
  uint32_t bleWritesForwarded;
  uint32_t scaleMainLinesSeen;
  uint32_t scaleCmdLinesSeen;
};

struct PbCommsEventHandlers {
  void (*onBleConnectionChanged)(bool connected) = nullptr;
  void (*onCommandReceived)(const PowderBotProtocol::PbCommand& command) = nullptr;
  void (*onScaleSampleReceived)(const PowderBotProtocol::PbScaleSample& sample) = nullptr;
  void (*onRawTrafficLine)(const char* channel, const char* line) = nullptr;
};

const PbCommsHubConfig& pbCommsDefaultConfig();
void pbCommsSetEventHandlers(const PbCommsEventHandlers& handlers);
void pbCommsBegin(const PbCommsHubConfig& config = pbCommsDefaultConfig());
void pbCommsUpdate();

bool pbCommsBleConnected();
const PbScaleLinkState& pbCommsScaleState();
const PbCommsHubStats& pbCommsStats();

bool pbCommsScaleSendRaw(const char* cmd);
bool pbCommsScaleCancelContinuous();
bool pbCommsScaleRequestImmediate();
bool pbCommsScaleRequestStable();
bool pbCommsScaleRequestImmediateAlt();
bool pbCommsScaleStartContinuous();
bool pbCommsHandleConsoleCommand(const char* line, Stream& out = Serial);

void pbCommsPrintBanner(Stream& out = Serial);
void pbCommsPrintState(Stream& out = Serial);
