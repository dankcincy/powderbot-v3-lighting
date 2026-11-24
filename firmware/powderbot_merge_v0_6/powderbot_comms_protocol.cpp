#include "powderbot_comms_protocol.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace PowderBotProtocol {

namespace {

static bool equalsIgnoreCase(const char* a, const char* b) {
  if (!a || !b) return false;
  while (*a && *b) {
    if (tolower(static_cast<unsigned char>(*a)) != tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

static void trimInPlace(char* s) {
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

} // namespace

const char* pbCommandTypeToString(PbCommandType type) {
  switch (type) {
    case PB_CMD_NONE: return "NONE";
    case PB_CMD_SET_TARGET_GN: return "SET_TARGET_GN";
    case PB_CMD_CLEAR_TARGET: return "CLEAR_TARGET";
    case PB_CMD_ARM: return "ARM";
    case PB_CMD_DISARM: return "DISARM";
    case PB_CMD_STOP: return "STOP";
    case PB_CMD_RUN_THROW: return "RUN_THROW";
    case PB_CMD_RUN_TRICKLER: return "RUN_TRICKLER";
    case PB_CMD_STOP_TRICKLER: return "STOP_TRICKLER";
    case PB_CMD_THROW_ON: return "THROW_ON";
    case PB_CMD_THROW_OFF: return "THROW_OFF";
    case PB_CMD_REQUEST_STATUS: return "REQUEST_STATUS";
    case PB_CMD_REQUEST_PARAMS: return "REQUEST_PARAMS";
    case PB_CMD_REQUEST_MACHINE_SNAPSHOT: return "REQUEST_MACHINE_SNAPSHOT";
    case PB_CMD_SCALE_REQUEST_IMMEDIATE: return "SCALE_REQUEST_IMMEDIATE";
    case PB_CMD_SCALE_REQUEST_STABLE: return "SCALE_REQUEST_STABLE";
    case PB_CMD_SCALE_STREAM_ON: return "SCALE_STREAM_ON";
    case PB_CMD_SCALE_STREAM_OFF: return "SCALE_STREAM_OFF";
    case PB_CMD_SCALE_CANCEL: return "SCALE_CANCEL";
    case PB_CMD_SCALE_TARE: return "SCALE_TARE";
    case PB_CMD_SCALE_ZERO: return "SCALE_ZERO";
    case PB_CMD_LEGACY_TEXT: return "LEGACY_TEXT";
    case PB_CMD_DEBUG_NOTE: return "DEBUG_NOTE";
    default: return "UNKNOWN";
  }
}

const char* pbEventTypeToString(PbEventType type) {
  switch (type) {
    case PB_EVENT_NONE: return "NONE";
    case PB_EVENT_BLE_CONNECTED: return "BLE_CONNECTED";
    case PB_EVENT_BLE_DISCONNECTED: return "BLE_DISCONNECTED";
    case PB_EVENT_LEGACY_APP_RX: return "LEGACY_APP_RX";
    case PB_EVENT_LEGACY_APP_TX: return "LEGACY_APP_TX";
    case PB_EVENT_COMMAND_ACCEPTED: return "COMMAND_ACCEPTED";
    case PB_EVENT_COMMAND_REJECTED: return "COMMAND_REJECTED";
    case PB_EVENT_TARGET_SET: return "TARGET_SET";
    case PB_EVENT_TARGET_CLEARED: return "TARGET_CLEARED";
    case PB_EVENT_MACHINE_MODE_CHANGED: return "MACHINE_MODE_CHANGED";
    case PB_EVENT_MACHINE_STOPPED: return "MACHINE_STOPPED";
    case PB_EVENT_THROW_ENABLED: return "THROW_ENABLED";
    case PB_EVENT_THROW_DISABLED: return "THROW_DISABLED";
    case PB_EVENT_TRICKLER_STARTED: return "TRICKLER_STARTED";
    case PB_EVENT_TRICKLER_STOPPED: return "TRICKLER_STOPPED";
    case PB_EVENT_SCALE_SAMPLE: return "SCALE_SAMPLE";
    case PB_EVENT_SCALE_STABLE: return "SCALE_STABLE";
    case PB_EVENT_SCALE_UNSTABLE: return "SCALE_UNSTABLE";
    case PB_EVENT_SCALE_STREAM_STARTED: return "SCALE_STREAM_STARTED";
    case PB_EVENT_SCALE_STREAM_STOPPED: return "SCALE_STREAM_STOPPED";
    case PB_EVENT_SCALE_ACK: return "SCALE_ACK";
    case PB_EVENT_SCALE_ERROR: return "SCALE_ERROR";
    case PB_EVENT_WARNING: return "WARNING";
    case PB_EVENT_FAULT: return "FAULT";
    case PB_EVENT_DEBUG_TEXT: return "DEBUG_TEXT";
    default: return "UNKNOWN";
  }
}

const char* pbMachineModeToString(PbMachineMode mode) {
  switch (mode) {
    case PB_MODE_IDLE: return "IDLE";
    case PB_MODE_READY: return "READY";
    case PB_MODE_ARMED: return "ARMED";
    case PB_MODE_THROWING: return "THROWING";
    case PB_MODE_TRICKLING: return "TRICKLING";
    case PB_MODE_SETTLING: return "SETTLING";
    case PB_MODE_COMPLETE: return "COMPLETE";
    case PB_MODE_STOPPED: return "STOPPED";
    case PB_MODE_FAULT: return "FAULT";
    default: return "UNKNOWN";
  }
}

const char* pbScaleLinkStateToString(PbScaleLinkState state) {
  switch (state) {
    case PB_SCALE_LINK_IDLE: return "IDLE";
    case PB_SCALE_LINK_WAITING_FOR_REPLY: return "WAITING_FOR_REPLY";
    case PB_SCALE_LINK_WAITING_FOR_STABLE: return "WAITING_FOR_STABLE";
    case PB_SCALE_LINK_STREAMING: return "STREAMING";
    case PB_SCALE_LINK_CANCEL_PENDING: return "CANCEL_PENDING";
    case PB_SCALE_LINK_FAULT: return "FAULT";
    default: return "UNKNOWN";
  }
}

bool pbIsScaleControlCommand(PbCommandType type) {
  switch (type) {
    case PB_CMD_SCALE_REQUEST_IMMEDIATE:
    case PB_CMD_SCALE_REQUEST_STABLE:
    case PB_CMD_SCALE_STREAM_ON:
    case PB_CMD_SCALE_STREAM_OFF:
    case PB_CMD_SCALE_CANCEL:
    case PB_CMD_SCALE_TARE:
    case PB_CMD_SCALE_ZERO:
      return true;
    default:
      return false;
  }
}

bool pbIsMachineControlCommand(PbCommandType type) {
  switch (type) {
    case PB_CMD_SET_TARGET_GN:
    case PB_CMD_CLEAR_TARGET:
    case PB_CMD_ARM:
    case PB_CMD_DISARM:
    case PB_CMD_STOP:
    case PB_CMD_RUN_THROW:
    case PB_CMD_RUN_TRICKLER:
    case PB_CMD_STOP_TRICKLER:
    case PB_CMD_THROW_ON:
    case PB_CMD_THROW_OFF:
      return true;
    default:
      return false;
  }
}

bool pbMakeLegacyCommandFromText(const char* text, PbCommand& out) {
  out = PbCommand{};
  if (!text || !text[0]) {
    return false;
  }

  char work[PB_LEGACY_TEXT_LEN] = {0};
  strncpy(work, text, sizeof(work) - 1);
  trimInPlace(work);
  if (!work[0]) {
    return false;
  }

  out.timestampMs = millis();
  strncpy(out.text, work, sizeof(out.text) - 1);

  if ((work[0] == 'T' || work[0] == 't') && work[1] != '\0') {
    out.type = PB_CMD_SET_TARGET_GN;
    out.valueGn = static_cast<float>(atof(work + 1));
    return true;
  }

  if (equalsIgnoreCase(work, "stop")) {
    out.type = PB_CMD_STOP;
    return true;
  }
  if (equalsIgnoreCase(work, "status")) {
    out.type = PB_CMD_REQUEST_STATUS;
    return true;
  }
  if (equalsIgnoreCase(work, "params")) {
    out.type = PB_CMD_REQUEST_PARAMS;
    return true;
  }
  if (equalsIgnoreCase(work, "run_throw")) {
    out.type = PB_CMD_RUN_THROW;
    return true;
  }
  if (equalsIgnoreCase(work, "run_trickler")) {
    out.type = PB_CMD_RUN_TRICKLER;
    return true;
  }
  if (equalsIgnoreCase(work, "stop_trickler")) {
    out.type = PB_CMD_STOP_TRICKLER;
    return true;
  }
  if (equalsIgnoreCase(work, "throw_on")) {
    out.type = PB_CMD_THROW_ON;
    return true;
  }
  if (equalsIgnoreCase(work, "throw_off")) {
    out.type = PB_CMD_THROW_OFF;
    return true;
  }

  // UI / future-app scale controls.
  if (equalsIgnoreCase(work, "scale_q") || equalsIgnoreCase(work, "scale_immediate")) {
    out.type = PB_CMD_SCALE_REQUEST_IMMEDIATE;
    return true;
  }
  if (equalsIgnoreCase(work, "scale_s") || equalsIgnoreCase(work, "scale_stable")) {
    out.type = PB_CMD_SCALE_REQUEST_STABLE;
    return true;
  }
  if (equalsIgnoreCase(work, "scale_stream_on") || equalsIgnoreCase(work, "scale_sir")) {
    out.type = PB_CMD_SCALE_STREAM_ON;
    return true;
  }
  if (equalsIgnoreCase(work, "scale_stream_off") || equalsIgnoreCase(work, "scale_c")) {
    out.type = PB_CMD_SCALE_CANCEL;
    return true;
  }
  if (equalsIgnoreCase(work, "scale_tare")) {
    out.type = PB_CMD_SCALE_TARE;
    return true;
  }
  if (equalsIgnoreCase(work, "scale_zero")) {
    out.type = PB_CMD_SCALE_ZERO;
    return true;
  }

  out.type = PB_CMD_LEGACY_TEXT;
  return true;
}

void pbFormatMachineSnapshot(const PbMachineState& state, char* out, size_t outLen) {
  if (!out || outLen == 0) {
    return;
  }

  snprintf(out, outLen,
           "mode=%s ble=%s target=%s targetGn=%.3f liveGn=%.3f errGn=%.3f throw=%s trickler=%s scale=%s stable=%s fault=%s",
           pbMachineModeToString(state.mode),
           state.bleConnected ? "on" : "off",
           state.targetValid ? "yes" : "no",
           state.targetGn,
           state.liveWeightGn,
           state.errorGn,
           state.throwEnabled ? "on" : "off",
           state.tricklerActive ? "on" : "off",
           pbScaleLinkStateToString(state.scaleLinkState),
           state.scaleStable ? "yes" : "no",
           state.faultActive ? "yes" : "no");
}

} // namespace PowderBotProtocol

