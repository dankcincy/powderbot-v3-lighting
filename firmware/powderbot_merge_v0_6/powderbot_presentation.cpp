#include "powderbot_presentation.h"

#include <Arduino.h>

PowderBotPresentation::PowderBotPresentation(const PbTimingConfig &config)
  : config_(config) {
}

void PowderBotPresentation::reset(bool bleConnected, uint32_t now) {
  (void)bleConnected;
  visualMode_ = PbVisualMode::Off;
  visualModeStartMs_ = now;
  lastAnimatedVerdict_ = PbVerdict::Off;
  displayedVerdict_ = PbVerdict::Off;
  pendingVerdict_ = PbVerdict::Off;
  pendingVerdictSinceMs_ = 0;
  stopVerdict_ = PbVerdict::Off;
  stopAnimStyle_ = PbStopAnimStyle::FullCollapse;
  wipeVerdict_ = PbVerdict::Off;
  startupPending_ = true;
  bleWakePending_ = false;
  bleDisconnectPending_ = false;
  stopAnimPending_ = false;
  targetAckPending_ = false;
  resetWipePending_ = false;
}

void PowderBotPresentation::beginMode(PbVisualMode newMode, uint32_t now, bool forceRestart) {
  if (!forceRestart && newMode == visualMode_) return;

  visualMode_ = newMode;
  visualModeStartMs_ = now;

  Serial.print("VISUAL -> ");
  Serial.println(pbVisualModeName(newMode));
}

void PowderBotPresentation::setFullShow(bool enabled) {
  config_.fullShow = enabled;
}

bool PowderBotPresentation::fullShowEnabled() const {
  return config_.fullShow;
}

void PowderBotPresentation::setVerdictFlipDelayMs(uint16_t delayMs) {
  verdictFlipDelayMs_ = delayMs;
}

uint16_t PowderBotPresentation::verdictFlipDelayMs() const {
  return verdictFlipDelayMs_;
}

PbVerdict PowderBotPresentation::displayedVerdict() const {
  return displayedVerdict_;
}

PbVerdict PowderBotPresentation::pendingVerdict() const {
  return pendingVerdict_;
}

PbVerdict PowderBotPresentation::effectiveDisplayedVerdict(const PbEngineSnapshot &snapshot) const {
  if (!snapshot.decisionActive) return PbVerdict::Off;
  if (displayedVerdict_ != PbVerdict::Off) return displayedVerdict_;
  return snapshot.verdict;
}

void PowderBotPresentation::updateDisplayedVerdict(const PbEngineSnapshot &snapshot, uint32_t now) {
  if (!snapshot.decisionActive || snapshot.verdict == PbVerdict::Off) {
    displayedVerdict_ = PbVerdict::Off;
    pendingVerdict_ = PbVerdict::Off;
    pendingVerdictSinceMs_ = 0;
    lastAnimatedVerdict_ = PbVerdict::Off;
    return;
  }

  const PbVerdict realVerdict = snapshot.verdict;

  if (displayedVerdict_ == PbVerdict::Off) {
    displayedVerdict_ = realVerdict;
    pendingVerdict_ = PbVerdict::Off;
    pendingVerdictSinceMs_ = 0;
    return;
  }

  if (realVerdict == displayedVerdict_) {
    pendingVerdict_ = PbVerdict::Off;
    pendingVerdictSinceMs_ = 0;
    return;
  }

  if (pendingVerdict_ != realVerdict) {
    pendingVerdict_ = realVerdict;
    pendingVerdictSinceMs_ = now;
    return;
  }

  if (verdictFlipDelayMs_ == 0 || (now - pendingVerdictSinceMs_) >= verdictFlipDelayMs_) {
    displayedVerdict_ = pendingVerdict_;
    pendingVerdict_ = PbVerdict::Off;
    pendingVerdictSinceMs_ = 0;
  }
}

PbVisualMode PowderBotPresentation::computeBaseVisualMode(const PbEngineSnapshot &snapshot) const {
  if (!snapshot.bleConnected) {
    return PbVisualMode::Off;
  }

  if (!snapshot.targetValid) {
    return PbVisualMode::IdleEmber;
  }

  if (snapshot.decisionActive) {
    return (effectiveDisplayedVerdict(snapshot) == PbVerdict::Pass) ? PbVisualMode::PassHold : PbVisualMode::FailHold;
  }

  if (!snapshot.runVisualLatched) {
    return PbVisualMode::Heartbeat;
  }

  return PbVisualMode::Scanner;
}

PbVisualMode PowderBotPresentation::computeMinimalVisualMode(const PbEngineSnapshot &snapshot) const {
  if (!snapshot.bleConnected) return PbVisualMode::Off;
  if (!snapshot.targetValid || !snapshot.decisionActive) return PbVisualMode::Off;
  return (effectiveDisplayedVerdict(snapshot) == PbVerdict::Pass) ? PbVisualMode::PassHold : PbVisualMode::FailHold;
}

void PowderBotPresentation::suppressNonVerdictPendingFlags() {
  startupPending_ = false;
  bleWakePending_ = false;
  bleDisconnectPending_ = false;
  stopAnimPending_ = false;
  targetAckPending_ = false;
}

void PowderBotPresentation::applyEngineEvents(const PbPendingEvents &events, const PbEngineSnapshot &, uint32_t now) {
  (void)now;
  if (events.connectEvent) {
    startupPending_ = false;
    bleWakePending_ = true;
    bleDisconnectPending_ = false;
  }

  if (events.disconnectEvent) {
    startupPending_ = false;
    targetAckPending_ = false;
    stopAnimPending_ = false;
    resetWipePending_ = false;
    bleWakePending_ = false;
    bleDisconnectPending_ = true;
    lastAnimatedVerdict_ = PbVerdict::Off;
    displayedVerdict_ = PbVerdict::Off;
    pendingVerdict_ = PbVerdict::Off;
    pendingVerdictSinceMs_ = 0;
  }

  if (events.stopReceived) {
    stopVerdict_ = events.stopVerdict;
    stopAnimStyle_ = events.stopAnimStyle;
    bleWakePending_ = false;
    targetAckPending_ = false;
    resetWipePending_ = false;
    stopAnimPending_ = true;
    lastAnimatedVerdict_ = PbVerdict::Off;
    displayedVerdict_ = PbVerdict::Off;
    pendingVerdict_ = PbVerdict::Off;
    pendingVerdictSinceMs_ = 0;
  }

  if (events.targetAccepted) {
    startupPending_ = false;
    stopAnimPending_ = false;
    resetWipePending_ = false;
    bleDisconnectPending_ = false;
    targetAckPending_ = true;
    lastAnimatedVerdict_ = PbVerdict::Off;
    displayedVerdict_ = PbVerdict::Off;
    pendingVerdict_ = PbVerdict::Off;
    pendingVerdictSinceMs_ = 0;
  }

  if (events.cupRemovalReset) {
    wipeVerdict_ = (events.wipeVerdict == PbVerdict::Pass) ? PbVerdict::Pass : PbVerdict::Fail;
    resetWipePending_ = true;
    lastAnimatedVerdict_ = PbVerdict::Off;
    displayedVerdict_ = PbVerdict::Off;
    pendingVerdict_ = PbVerdict::Off;
    pendingVerdictSinceMs_ = 0;
  }
}

bool PowderBotPresentation::modeComplete(uint32_t now) const {
  const uint32_t elapsed = now - visualModeStartMs_;

  switch (visualMode_) {
    case PbVisualMode::Startup:
      return elapsed >= config_.startupDurationMs;
    case PbVisualMode::BleWake:
      return elapsed >= config_.bleWakeDurationMs;
    case PbVisualMode::BleDisconnect:
      return elapsed >= config_.bleDisconnectDurationMs;
    case PbVisualMode::StopAnim:
      return elapsed >= config_.stopAnimDurationMs;
    case PbVisualMode::TargetAck:
      return elapsed >= config_.targetAckDurationMs;
    case PbVisualMode::PassAnim:
    case PbVisualMode::FailAnim:
    case PbVisualMode::ResetWipe: {
      const uint16_t pairs = config_.pixelCount / 2;
      const uint32_t totalDuration = (pairs + 1) * config_.resetWipeStepMs;
      return elapsed >= totalDuration;
    }
    default:
      return false;
  }
}

void PowderBotPresentation::update(const PbEngineSnapshot &snapshot, uint32_t now) {
  updateDisplayedVerdict(snapshot, now);

  if (!config_.fullShow) {
    suppressNonVerdictPendingFlags();

    if (resetWipePending_ || visualMode_ == PbVisualMode::ResetWipe) {
      if (visualMode_ != PbVisualMode::ResetWipe) {
        beginMode(PbVisualMode::ResetWipe, now, true);
      }
      if (modeComplete(now)) {
        resetWipePending_ = false;
        beginMode(computeMinimalVisualMode(snapshot), now, true);
      }
      return;
    }

    const PbVerdict visualVerdict = effectiveDisplayedVerdict(snapshot);
    if (visualVerdict == PbVerdict::Pass && lastAnimatedVerdict_ != PbVerdict::Pass) {
      lastAnimatedVerdict_ = PbVerdict::Pass;
      beginMode(PbVisualMode::PassAnim, now, true);
      return;
    } else if (visualVerdict == PbVerdict::Fail && lastAnimatedVerdict_ != PbVerdict::Fail) {
      lastAnimatedVerdict_ = PbVerdict::Fail;
      beginMode(PbVisualMode::FailAnim, now, true);
      return;
    } else if (visualVerdict == PbVerdict::Off) {
      lastAnimatedVerdict_ = PbVerdict::Off;
    }

    if (visualMode_ == PbVisualMode::PassAnim) {
      if (modeComplete(now)) {
        beginMode(PbVisualMode::PassHold, now, true);
      }
      return;
    }

    if (visualMode_ == PbVisualMode::FailAnim) {
      if (modeComplete(now)) {
        beginMode(PbVisualMode::FailHold, now, true);
      }
      return;
    }

    const PbVisualMode baseMode = computeMinimalVisualMode(snapshot);
    if (visualMode_ != baseMode) {
      beginMode(baseMode, now, true);
    }
    return;
  }

  if (snapshot.targetValid) {
    startupPending_ = false;
  }

  if (bleDisconnectPending_ || visualMode_ == PbVisualMode::BleDisconnect) {
    if (visualMode_ != PbVisualMode::BleDisconnect) {
      beginMode(PbVisualMode::BleDisconnect, now, true);
    }
    if (modeComplete(now)) {
      bleDisconnectPending_ = false;
      beginMode(computeBaseVisualMode(snapshot), now, true);
    }
    return;
  }

  if (startupPending_ && !snapshot.targetValid && !snapshot.bleConnected) {
    if (visualMode_ != PbVisualMode::Startup) {
      beginMode(PbVisualMode::Startup, now, true);
    }
    if (modeComplete(now)) {
      startupPending_ = false;
      beginMode(computeBaseVisualMode(snapshot), now, true);
    }
    return;
  }

  if (stopAnimPending_ || visualMode_ == PbVisualMode::StopAnim) {
    if (visualMode_ != PbVisualMode::StopAnim) {
      beginMode(PbVisualMode::StopAnim, now, true);
    }
    if (modeComplete(now)) {
      stopAnimPending_ = false;
      beginMode(computeBaseVisualMode(snapshot), now, true);
    }
    return;
  }

  if (bleWakePending_ || visualMode_ == PbVisualMode::BleWake) {
    if (visualMode_ != PbVisualMode::BleWake) {
      beginMode(PbVisualMode::BleWake, now, true);
    }
    if (modeComplete(now)) {
      bleWakePending_ = false;
      beginMode(computeBaseVisualMode(snapshot), now, true);
    }
    return;
  }

  if (targetAckPending_ || visualMode_ == PbVisualMode::TargetAck) {
    if (visualMode_ != PbVisualMode::TargetAck) {
      beginMode(PbVisualMode::TargetAck, now, true);
    }
    if (modeComplete(now)) {
      targetAckPending_ = false;
      beginMode(computeBaseVisualMode(snapshot), now, true);
    }
    return;
  }

  if (resetWipePending_ || visualMode_ == PbVisualMode::ResetWipe) {
    if (visualMode_ != PbVisualMode::ResetWipe) {
      beginMode(PbVisualMode::ResetWipe, now, true);
    }
    if (modeComplete(now)) {
      resetWipePending_ = false;
      beginMode(computeBaseVisualMode(snapshot), now, true);
    }
    return;
  }

  const PbVerdict visualVerdict = effectiveDisplayedVerdict(snapshot);
  if (visualVerdict == PbVerdict::Pass && lastAnimatedVerdict_ != PbVerdict::Pass) {
    lastAnimatedVerdict_ = PbVerdict::Pass;
    beginMode(PbVisualMode::PassAnim, now, true);
    return;
  } else if (visualVerdict == PbVerdict::Fail && lastAnimatedVerdict_ != PbVerdict::Fail) {
    lastAnimatedVerdict_ = PbVerdict::Fail;
    beginMode(PbVisualMode::FailAnim, now, true);
    return;
  } else if (visualVerdict == PbVerdict::Off) {
    lastAnimatedVerdict_ = PbVerdict::Off;
  }

  if (visualMode_ == PbVisualMode::PassAnim) {
    if (modeComplete(now)) {
      beginMode(PbVisualMode::PassHold, now, true);
    }
    return;
  }

  if (visualMode_ == PbVisualMode::FailAnim) {
    if (modeComplete(now)) {
      beginMode(PbVisualMode::FailHold, now, true);
    }
    return;
  }

  const PbVisualMode baseMode = computeBaseVisualMode(snapshot);
  if (visualMode_ != baseMode) {
    beginMode(baseMode, now, true);
  }
}

PbRenderState PowderBotPresentation::renderState() const {
  PbRenderState state;
  state.mode = visualMode_;
  state.modeStartMs = visualModeStartMs_;
  state.stopVerdict = stopVerdict_;
  state.stopAnimStyle = stopAnimStyle_;
  state.wipeVerdict = wipeVerdict_;
  return state;
}
