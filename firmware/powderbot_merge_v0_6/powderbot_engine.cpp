#include "powderbot_engine.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

PowderBotEngine::PowderBotEngine(const PbEngineConfig &config)
  : config_(config) {
}

uint8_t PowderBotEngine::decisionSampleCount() const {
  const uint8_t maxSamples = static_cast<uint8_t>(sizeof(stableWindow_) / sizeof(stableWindow_[0]));
  if (config_.decisionSamples == 0) return 1;
  return (config_.decisionSamples > maxSamples) ? maxSamples : config_.decisionSamples;
}

void PowderBotEngine::reset(bool bleConnected) {
  bleConnected_ = bleConnected;
  targetValid_ = false;
  targetWeightGn_ = 0.0f;
  liveStable_ = false;
  liveWeightGn_ = 0.0f;
  clearRunStateOnly();
  pendingEvents_ = PbPendingEvents();
}

void PowderBotEngine::clearDecisionWindow() {
  stableWindowCount_ = 0;
  memset(stableWindow_, 0, sizeof(stableWindow_));
}

void PowderBotEngine::clearRunStateOnly() {
  clearDecisionWindow();
  decisionActive_ = false;
  displayedWeightGn_ = 0.0f;
  cupRemovalCounter_ = 0;
  runVisualLatched_ = false;
  runStartCounter_ = 0;
  lastLoggedVerdict_ = PbVerdict::Off;
}

bool PowderBotEngine::isCupRemovalReading() const {
  return liveWeightGn_ < config_.cupRemovalThresholdGn;
}

bool PowderBotEngine::isDuplicateTarget(float newTargetGn) const {
  return targetValid_ && fabsf(newTargetGn - targetWeightGn_) <= config_.targetDuplicateEpsilonGn;
}

bool PowderBotEngine::isWithinAcceptanceWindow(float weightGn) const {
  const float minAcceptableGn = targetWeightGn_ - config_.underToleranceGn;
  const float maxAcceptableGn = targetWeightGn_ + config_.overToleranceGn;
  return (weightGn >= minAcceptableGn) && (weightGn <= maxAcceptableGn);
}

PbVerdict PowderBotEngine::currentVerdict() const {
  if (!targetValid_ || !decisionActive_) return PbVerdict::Off;

  return isWithinAcceptanceWindow(displayedWeightGn_) ? PbVerdict::Pass : PbVerdict::Fail;
}


void PowderBotEngine::setUnderToleranceGn(float underToleranceGn) {
  if (underToleranceGn < 0.0f) underToleranceGn = 0.0f;
  config_.underToleranceGn = underToleranceGn;
}

void PowderBotEngine::setOverToleranceGn(float overToleranceGn) {
  if (overToleranceGn < 0.0f) overToleranceGn = 0.0f;
  config_.overToleranceGn = overToleranceGn;
}

void PowderBotEngine::setVerdictTolerancesGn(float underToleranceGn, float overToleranceGn) {
  setUnderToleranceGn(underToleranceGn);
  setOverToleranceGn(overToleranceGn);
}

float PowderBotEngine::underToleranceGn() const {
  return config_.underToleranceGn;
}

float PowderBotEngine::overToleranceGn() const {
  return config_.overToleranceGn;
}

PbMachineState PowderBotEngine::currentMachineState() const {
  if (!bleConnected_) return PbMachineState::Disconnected;
  if (!targetValid_) return PbMachineState::ConnectedIdle;
  if (decisionActive_) {
    return (currentVerdict() == PbVerdict::Pass) ? PbMachineState::ResultPass : PbMachineState::ResultFail;
  }
  if (runVisualLatched_) return PbMachineState::Running;
  return PbMachineState::Armed;
}

void PowderBotEngine::logVerdictTransition() {
  const PbVerdict verdict = currentVerdict();
  if (verdict == lastLoggedVerdict_) return;

  lastLoggedVerdict_ = verdict;
  Serial.print("VERDICT -> ");
  Serial.println(pbVerdictName(verdict));
}

void PowderBotEngine::onBleConnectionChanged(bool connected) {
  if (connected == bleConnected_) return;

  bleConnected_ = connected;

  Serial.print("BLE STATE -> ");
  Serial.println(bleConnected_ ? "CONNECTED" : "DISCONNECTED");

  if (bleConnected_) {
    pendingEvents_.connectEvent = true;
    pendingEvents_.disconnectEvent = false;
    return;
  }

  targetWeightGn_ = 0.0f;
  targetValid_ = false;
  clearRunStateOnly();
  pendingEvents_.disconnectEvent = true;
  pendingEvents_.connectEvent = false;
}

void PowderBotEngine::onStopCommand() {
  const bool hadResult = decisionActive_;
  const PbVerdict stopVerdict = hadResult ? currentVerdict() : PbVerdict::Fail;

  Serial.print("BLE stop received -> ");
  if (stopVerdict == PbVerdict::Pass) {
    Serial.println("green disarm collapse -> connected idle ember / off");
  } else if (hadResult) {
    Serial.println("red disarm collapse -> connected idle ember / off");
  } else {
    Serial.println("red center-pinch disarm -> connected idle ember / off");
  }

  targetWeightGn_ = 0.0f;
  targetValid_ = false;
  clearRunStateOnly();

  pendingEvents_.stopReceived = true;
  pendingEvents_.stopVerdict = stopVerdict;
  pendingEvents_.stopAnimStyle = hadResult ? PbStopAnimStyle::FullCollapse : PbStopAnimStyle::CenterPinch;
}

bool PowderBotEngine::onTargetReceived(float newTargetGn) {
  if (newTargetGn <= 0.0f) return false;

  if (isDuplicateTarget(newTargetGn)) {
    Serial.print("TARGET DUPLICATE IGNORED: ");
    Serial.println(newTargetGn, 3);
    return false;
  }

  targetWeightGn_ = newTargetGn;
  targetValid_ = true;
  clearRunStateOnly();
  pendingEvents_.targetAccepted = true;

  Serial.print("TARGET CAPTURED: ");
  Serial.println(targetWeightGn_, 3);
  return true;
}

void PowderBotEngine::pushStableSample(float w) {
  const uint8_t sampleCount = decisionSampleCount();

  if (stableWindowCount_ < sampleCount) {
    stableWindow_[stableWindowCount_++] = w;
    return;
  }

  for (uint8_t i = 1; i < sampleCount; ++i) {
    stableWindow_[i - 1] = stableWindow_[i];
  }
  stableWindow_[sampleCount - 1] = w;
}

bool PowderBotEngine::evaluateDecisionWindow(float &avgOut, float &spreadOut) const {
  const uint8_t sampleCount = decisionSampleCount();
  if (stableWindowCount_ < sampleCount) return false;

  float minW = stableWindow_[0];
  float maxW = stableWindow_[0];
  float sumW = 0.0f;

  for (uint8_t i = 0; i < sampleCount; ++i) {
    const float w = stableWindow_[i];
    if (w < minW) minW = w;
    if (w > maxW) maxW = w;
    sumW += w;
  }

  avgOut = sumW / sampleCount;
  spreadOut = maxW - minW;
  return (spreadOut <= config_.decisionMaxSpreadGn);
}

void PowderBotEngine::checkForRollingSettledDecision() {
  if (!targetValid_) return;

  if (decisionActive_ && isCupRemovalReading()) {
    return;
  }

  if (!liveStable_) {
    return;
  }

  const float errorGn = liveWeightGn_ - targetWeightGn_;
  if (!decisionActive_ && errorGn < -config_.armBelowTargetGn) {
    clearDecisionWindow();
    return;
  }

  pushStableSample(liveWeightGn_);

  float avgW = 0.0f;
  float spreadW = 0.0f;
  const bool windowTightEnough = evaluateDecisionWindow(avgW, spreadW);

  Serial.print("WINDOW samples=");
  Serial.print(stableWindowCount_);
  Serial.print(" avg=");
  Serial.print(avgW, 2);
  Serial.print(" spread=");
  Serial.println(spreadW, 3);

  if (!windowTightEnough) return;

  displayedWeightGn_ = avgW;

  if (!decisionActive_) {
    decisionActive_ = true;
    Serial.print("SETTLED MONITOR ARMED @ ");
    Serial.print(displayedWeightGn_, 2);
    Serial.println(" gn");
  } else {
    Serial.print("SETTLED UPDATE @ ");
    Serial.print(displayedWeightGn_, 2);
    Serial.println(" gn");
  }

  Serial.print("ROLLING RESULT -> ");
  Serial.println(isWithinAcceptanceWindow(displayedWeightGn_) ? "GREEN" : "RED");
}

void PowderBotEngine::checkForCupRemovalReset() {
  if (!targetValid_ || !decisionActive_) return;

  if (liveWeightGn_ < config_.cupRemovalThresholdGn) {
    if (cupRemovalCounter_ < config_.cupRemovalCount) {
      ++cupRemovalCounter_;
    }
  } else {
    cupRemovalCounter_ = 0;
  }

  Serial.print("CUP RESET counter=");
  Serial.print(cupRemovalCounter_);
  Serial.print(" weight=");
  Serial.println(liveWeightGn_, 2);

  if (cupRemovalCounter_ < config_.cupRemovalCount) return;

  const PbVerdict wipeVerdict = currentVerdict();
  Serial.println("CUP REMOVED -> wipe to black / clear settled monitor / re-arm same target");

  clearRunStateOnly();
  pendingEvents_.cupRemovalReset = true;
  pendingEvents_.wipeVerdict = (wipeVerdict == PbVerdict::Pass) ? PbVerdict::Pass : PbVerdict::Fail;
}

void PowderBotEngine::onScaleSample(bool stable, float weightGn) {
  liveStable_ = stable;
  liveWeightGn_ = weightGn;

  if (targetValid_ && !decisionActive_ && !runVisualLatched_) {
    if (liveWeightGn_ >= config_.runStartThresholdGn) {
      if (runStartCounter_ < config_.runStartCount) {
        ++runStartCounter_;
      }
    } else {
      runStartCounter_ = 0;
    }

    if (runStartCounter_ >= config_.runStartCount) {
      runVisualLatched_ = true;
      Serial.println("RUN VISUAL -> SCANNER LATCHED");
    }
  }

  Serial.print("SCALE: ");
  Serial.print(stable ? "ST " : "US ");
  Serial.print(liveWeightGn_, 2);
  Serial.println(" gn");

  if (targetValid_) {
    const float errorGn = liveWeightGn_ - targetWeightGn_;
    Serial.print("ERROR: ");
    Serial.println(errorGn, 2);
  }

  checkForCupRemovalReset();
  if (pendingEvents_.cupRemovalReset) {
    logVerdictTransition();
    return;
  }

  if (targetValid_ && decisionActive_ && isCupRemovalReading()) {
    logVerdictTransition();
    return;
  }

  checkForRollingSettledDecision();
  logVerdictTransition();
}

PbPendingEvents PowderBotEngine::consumePendingEvents() {
  const PbPendingEvents out = pendingEvents_;
  pendingEvents_ = PbPendingEvents();
  return out;
}

PbEngineSnapshot PowderBotEngine::snapshot() const {
  PbEngineSnapshot snapshot;
  snapshot.bleConnected = bleConnected_;
  snapshot.targetValid = targetValid_;
  snapshot.liveStable = liveStable_;
  snapshot.decisionActive = decisionActive_;
  snapshot.runVisualLatched = runVisualLatched_;
  snapshot.targetWeightGn = targetWeightGn_;
  snapshot.liveWeightGn = liveWeightGn_;
  snapshot.displayedWeightGn = displayedWeightGn_;
  snapshot.verdict = currentVerdict();
  snapshot.machineState = currentMachineState();
  return snapshot;
}

