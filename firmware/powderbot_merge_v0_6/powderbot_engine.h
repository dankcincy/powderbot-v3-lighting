#pragma once

#include "powderbot_types.h"

class PowderBotEngine {
public:
  explicit PowderBotEngine(const PbEngineConfig &config);

  void reset(bool bleConnected);
  void onBleConnectionChanged(bool connected);
  void onStopCommand();
  bool onTargetReceived(float newTargetGn);
  void onScaleSample(bool stable, float weightGn);

  void setUnderToleranceGn(float underToleranceGn);
  void setOverToleranceGn(float overToleranceGn);
  void setVerdictTolerancesGn(float underToleranceGn, float overToleranceGn);
  float underToleranceGn() const;
  float overToleranceGn() const;

  PbPendingEvents consumePendingEvents();
  PbEngineSnapshot snapshot() const;

private:
  uint8_t decisionSampleCount() const;
  void clearDecisionWindow();
  void clearRunStateOnly();
  void pushStableSample(float w);
  bool evaluateDecisionWindow(float &avgOut, float &spreadOut) const;
  void checkForCupRemovalReset();
  void checkForRollingSettledDecision();
  bool isCupRemovalReading() const;
  bool isDuplicateTarget(float newTargetGn) const;
  bool isWithinAcceptanceWindow(float weightGn) const;
  PbVerdict currentVerdict() const;
  PbMachineState currentMachineState() const;
  void logVerdictTransition();

  PbEngineConfig config_;

  bool bleConnected_ = false;
  bool targetValid_ = false;
  float targetWeightGn_ = 0.0f;

  bool liveStable_ = false;
  float liveWeightGn_ = 0.0f;

  float stableWindow_[8] = {0.0f};
  uint8_t stableWindowCount_ = 0;
  bool decisionActive_ = false;
  float displayedWeightGn_ = 0.0f;
  uint8_t cupRemovalCounter_ = 0;
  bool runVisualLatched_ = false;
  uint8_t runStartCounter_ = 0;
  PbVerdict lastLoggedVerdict_ = PbVerdict::Off;

  PbPendingEvents pendingEvents_;
};
