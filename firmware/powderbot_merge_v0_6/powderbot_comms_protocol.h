#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

// PowderBot internal protocol contract.
// This file defines the app/UI/comms/core-facing command, event, and state
// structures so transport details stay out of machine logic.

namespace PowderBotProtocol {

constexpr size_t PB_TEXT_SMALL_LEN   = 32;
constexpr size_t PB_TEXT_MEDIUM_LEN   = 64;
constexpr size_t PB_TEXT_LARGE_LEN    = 96;
constexpr size_t PB_LEGACY_TEXT_LEN   = 64;

// -----------------------------
// Core command model
// -----------------------------

enum PbCommandType : uint8_t {
  PB_CMD_NONE = 0,

  // Machine control
  PB_CMD_SET_TARGET_GN,
  PB_CMD_CLEAR_TARGET,
  PB_CMD_ARM,
  PB_CMD_DISARM,
  PB_CMD_STOP,
  PB_CMD_RUN_THROW,
  PB_CMD_RUN_TRICKLER,
  PB_CMD_STOP_TRICKLER,
  PB_CMD_THROW_ON,
  PB_CMD_THROW_OFF,

  // Introspection / sync
  PB_CMD_REQUEST_STATUS,
  PB_CMD_REQUEST_PARAMS,
  PB_CMD_REQUEST_MACHINE_SNAPSHOT,

  // Scale-link actions
  PB_CMD_SCALE_REQUEST_IMMEDIATE,
  PB_CMD_SCALE_REQUEST_STABLE,
  PB_CMD_SCALE_STREAM_ON,
  PB_CMD_SCALE_STREAM_OFF,
  PB_CMD_SCALE_CANCEL,
  PB_CMD_SCALE_TARE,
  PB_CMD_SCALE_ZERO,

  // Service / debug / passthrough
  PB_CMD_LEGACY_TEXT,
  PB_CMD_DEBUG_NOTE
};

struct PbCommand {
  PbCommandType type = PB_CMD_NONE;
  uint32_t timestampMs = 0;

  // Optional numeric payloads.
  float valueGn = 0.0f;       // usually target grains or scale reading request context
  int32_t valueInt = 0;       // generic integer payload / flags
  bool valueBool = false;     // generic boolean payload

  // Optional legacy/raw text payload for adapters.
  char text[PB_LEGACY_TEXT_LEN] = {0};
};

// -----------------------------
// Scale model
// -----------------------------

enum PbScaleLinkState : uint8_t {
  PB_SCALE_LINK_IDLE = 0,
  PB_SCALE_LINK_WAITING_FOR_REPLY,
  PB_SCALE_LINK_WAITING_FOR_STABLE,
  PB_SCALE_LINK_STREAMING,
  PB_SCALE_LINK_CANCEL_PENDING,
  PB_SCALE_LINK_FAULT
};

struct PbScaleSample {
  bool valid = false;
  bool stable = false;
  float weightGn = 0.0f;
  bool overload = false;
  bool underload = false;
  uint32_t timestampMs = 0;
  char raw[PB_TEXT_SMALL_LEN] = {0};
};

// -----------------------------
// Machine state model
// -----------------------------

enum PbMachineMode : uint8_t {
  PB_MODE_IDLE = 0,
  PB_MODE_READY,
  PB_MODE_ARMED,
  PB_MODE_THROWING,
  PB_MODE_TRICKLING,
  PB_MODE_SETTLING,
  PB_MODE_COMPLETE,
  PB_MODE_STOPPED,
  PB_MODE_FAULT
};

struct PbMachineState {
  PbMachineMode mode = PB_MODE_IDLE;

  bool bleConnected = false;
  bool targetValid = false;
  bool throwEnabled = false;
  bool tricklerActive = false;
  bool bulkThrowActive = false;
  bool scaleStreaming = false;
  bool scaleStable = false;
  bool faultActive = false;

  float targetGn = 0.0f;
  float liveWeightGn = 0.0f;
  float errorGn = 0.0f;

  PbScaleLinkState scaleLinkState = PB_SCALE_LINK_IDLE;

  uint32_t timestampMs = 0;
  char faultText[PB_TEXT_MEDIUM_LEN] = {0};
  char statusText[PB_TEXT_MEDIUM_LEN] = {0};
};

// -----------------------------
// Event model
// -----------------------------

enum PbEventType : uint8_t {
  PB_EVENT_NONE = 0,

  // Link / session events
  PB_EVENT_BLE_CONNECTED,
  PB_EVENT_BLE_DISCONNECTED,
  PB_EVENT_LEGACY_APP_RX,
  PB_EVENT_LEGACY_APP_TX,

  // Core machine events
  PB_EVENT_COMMAND_ACCEPTED,
  PB_EVENT_COMMAND_REJECTED,
  PB_EVENT_TARGET_SET,
  PB_EVENT_TARGET_CLEARED,
  PB_EVENT_MACHINE_MODE_CHANGED,
  PB_EVENT_MACHINE_STOPPED,
  PB_EVENT_THROW_ENABLED,
  PB_EVENT_THROW_DISABLED,
  PB_EVENT_TRICKLER_STARTED,
  PB_EVENT_TRICKLER_STOPPED,

  // Scale events
  PB_EVENT_SCALE_SAMPLE,
  PB_EVENT_SCALE_STABLE,
  PB_EVENT_SCALE_UNSTABLE,
  PB_EVENT_SCALE_STREAM_STARTED,
  PB_EVENT_SCALE_STREAM_STOPPED,
  PB_EVENT_SCALE_ACK,
  PB_EVENT_SCALE_ERROR,

  // Diagnostics / faults
  PB_EVENT_WARNING,
  PB_EVENT_FAULT,
  PB_EVENT_DEBUG_TEXT
};

struct PbEvent {
  PbEventType type = PB_EVENT_NONE;
  uint32_t timestampMs = 0;

  float valueGn = 0.0f;
  int32_t valueInt = 0;
  bool valueBool = false;

  PbMachineMode machineMode = PB_MODE_IDLE;
  PbScaleSample scaleSample;

  char text[PB_TEXT_LARGE_LEN] = {0};
};

// -----------------------------
// Helpers / parsing entry points
// -----------------------------

const char* pbCommandTypeToString(PbCommandType type);
const char* pbEventTypeToString(PbEventType type);
const char* pbMachineModeToString(PbMachineMode mode);
const char* pbScaleLinkStateToString(PbScaleLinkState state);

bool pbIsScaleControlCommand(PbCommandType type);
bool pbIsMachineControlCommand(PbCommandType type);

bool pbMakeLegacyCommandFromText(const char* text, PbCommand& out);
void pbFormatMachineSnapshot(const PbMachineState& state, char* out, size_t outLen);

} // namespace PowderBotProtocol

