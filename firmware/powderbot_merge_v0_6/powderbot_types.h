#pragma once

#include <Arduino.h>

struct PbEngineConfig {
  float underToleranceGn;
  float overToleranceGn;
  float armBelowTargetGn;
  uint8_t decisionSamples;
  float decisionMaxSpreadGn;
  float runStartThresholdGn;
  uint8_t runStartCount;
  float cupRemovalThresholdGn;
  uint8_t cupRemovalCount;
  float targetDuplicateEpsilonGn;
};

struct PbTimingConfig {
  bool fullShow;
  uint16_t startupDurationMs;
  uint16_t bleWakeDurationMs;
  uint16_t bleDisconnectDurationMs;
  uint16_t stopAnimDurationMs;
  uint16_t idleEmberPeriodMs;
  uint16_t heartbeatPeriodMs;
  uint16_t targetAckDurationMs;
  uint16_t scannerFrameMs;
  uint16_t passAnimDurationMs;
  uint16_t failAnimDurationMs;
  uint16_t resetWipeStepMs;
  uint16_t pixelCount;
};

enum class PbVerdict : uint8_t {
  Off,
  Pass,
  Fail
};

enum class PbMachineState : uint8_t {
  Disconnected,
  ConnectedIdle,
  Armed,
  Running,
  ResultPass,
  ResultFail
};

enum class PbVisualMode : uint8_t {
  Off,
  Startup,
  BleWake,
  BleDisconnect,
  StopAnim,
  IdleEmber,
  Heartbeat,
  TargetAck,
  Scanner,
  PassAnim,
  PassHold,
  FailAnim,
  FailHold,
  ResetWipe
};

enum class PbStopAnimStyle : uint8_t {
  FullCollapse,
  CenterPinch
};

struct PbEngineSnapshot {
  bool bleConnected;
  bool targetValid;
  bool liveStable;
  bool decisionActive;
  bool runVisualLatched;
  float targetWeightGn;
  float liveWeightGn;
  float displayedWeightGn;
  PbVerdict verdict;
  PbMachineState machineState;
};

struct PbPendingEvents {
  bool connectEvent = false;
  bool disconnectEvent = false;
  bool targetAccepted = false;
  bool stopReceived = false;
  bool cupRemovalReset = false;
  PbVerdict stopVerdict = PbVerdict::Off;
  PbStopAnimStyle stopAnimStyle = PbStopAnimStyle::FullCollapse;
  PbVerdict wipeVerdict = PbVerdict::Off;
};

struct PbRenderState {
  PbVisualMode mode = PbVisualMode::Off;
  uint32_t modeStartMs = 0;
  PbVerdict stopVerdict = PbVerdict::Off;
  PbStopAnimStyle stopAnimStyle = PbStopAnimStyle::FullCollapse;
  PbVerdict wipeVerdict = PbVerdict::Off;
};

inline const char *pbVerdictName(PbVerdict verdict) {
  switch (verdict) {
    case PbVerdict::Off:  return "OFF";
    case PbVerdict::Pass: return "GREEN";
    case PbVerdict::Fail: return "RED";
  }
  return "?";
}

inline const char *pbVisualModeName(PbVisualMode mode) {
  switch (mode) {
    case PbVisualMode::Off:           return "OFF";
    case PbVisualMode::Startup:       return "STARTUP";
    case PbVisualMode::BleWake:       return "BLE WAKE";
    case PbVisualMode::BleDisconnect: return "BLE DISCONNECT";
    case PbVisualMode::StopAnim:      return "STOP ANIM";
    case PbVisualMode::IdleEmber:     return "IDLE EMBER";
    case PbVisualMode::Heartbeat:     return "HEARTBEAT";
    case PbVisualMode::TargetAck:     return "TARGET ACK";
    case PbVisualMode::Scanner:       return "SCANNER";
    case PbVisualMode::PassAnim:      return "PASS ANIM";
    case PbVisualMode::PassHold:      return "PASS HOLD";
    case PbVisualMode::FailAnim:      return "FAIL ANIM";
    case PbVisualMode::FailHold:      return "FAIL HOLD";
    case PbVisualMode::ResetWipe:     return "RESET WIPE";
  }
  return "?";
}
