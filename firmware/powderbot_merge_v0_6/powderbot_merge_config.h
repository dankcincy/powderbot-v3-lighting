#pragma once

#include "powderbot_types.h"

namespace PowderBotMergeConfig {
  // Display / local-output side.
  static const int STRIP_DATA_PIN = 21;
  static const uint16_t NUM_PIXELS = 22;
  static const uint8_t LED_BRIGHTNESS = 100;

  // USB serial for bench visibility.
  static const uint32_t USB_BAUD = 115200;

  // Bench trace controls. Keep these at the edge so modules stay portable.
  static const bool TRACE_RAW_TRAFFIC = true;
  static const bool TRACE_UNHANDLED_COMMANDS = true;

  // Comms tap/debug defaults. Keep them here so configureCommsHub() only applies policy
  // instead of hardwiring bench chatter inside the transport module.
  static const bool COMMS_TRAFFIC_TAP_ENABLED = true;
  static const bool COMMS_TAP_APP_TO_V3 = true;
  static const bool COMMS_TAP_V3_TO_APP = true;
  static const bool COMMS_TAP_SCALE_MAIN = true;
  static const bool COMMS_TAP_SCALE_CMD = true;
  static const bool COMMS_TAP_SCALE_SUMMARY = true;

  // Easy verdict-tolerance knobs.
  static const float UNDER_TOLERANCE_GN = 0.02f;
  static const float OVER_TOLERANCE_GN  = 0.02f;

  // Visual-only shock absorber for pass/fail flips.
  static const uint16_t VISUAL_VERDICT_FLIP_DELAY_MS = 500;

  // Engine and presentation config donated from powderbot_core_v2_2.
  static const PbEngineConfig ENGINE_CONFIG = {
    UNDER_TOLERANCE_GN,  // underToleranceGn
    OVER_TOLERANCE_GN,   // overToleranceGn
    0.08f,               // armBelowTargetGn
    4,       // decisionSamples
    0.03f,   // decisionMaxSpreadGn
    1.00f,   // runStartThresholdGn
    2,       // runStartCount
    1.00f,   // cupRemovalThresholdGn
    3,       // cupRemovalCount
    0.0005f  // targetDuplicateEpsilonGn
  };

  static const PbTimingConfig TIMING_CONFIG = {
    true,  // fullShow
    850,   // startupDurationMs
    620,   // bleWakeDurationMs
    340,   // bleDisconnectDurationMs
    340,   // stopAnimDurationMs
    2000,  // idleEmberPeriodMs
    1400,  // heartbeatPeriodMs
    520,   // targetAckDurationMs
    90,    // scannerFrameMs
    260,   // passAnimDurationMs
    260,   // failAnimDurationMs
    36,    // resetWipeStepMs
    NUM_PIXELS
  };
}
