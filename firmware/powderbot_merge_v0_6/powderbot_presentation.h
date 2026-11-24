#pragma once

#include "powderbot_types.h"

class PowderBotPresentation {
public:
  explicit PowderBotPresentation(const PbTimingConfig &config);

  void reset(bool bleConnected, uint32_t now);
  void applyEngineEvents(const PbPendingEvents &events, const PbEngineSnapshot &snapshot, uint32_t now);
  void update(const PbEngineSnapshot &snapshot, uint32_t now);
  void setFullShow(bool enabled);
  bool fullShowEnabled() const;
  void setVerdictFlipDelayMs(uint16_t delayMs);
  uint16_t verdictFlipDelayMs() const;
  PbVerdict displayedVerdict() const;
  PbVerdict pendingVerdict() const;
  PbRenderState renderState() const;

private:
  PbVisualMode computeBaseVisualMode(const PbEngineSnapshot &snapshot) const;
  PbVisualMode computeMinimalVisualMode(const PbEngineSnapshot &snapshot) const;
  PbVerdict effectiveDisplayedVerdict(const PbEngineSnapshot &snapshot) const;
  void updateDisplayedVerdict(const PbEngineSnapshot &snapshot, uint32_t now);
  void suppressNonVerdictPendingFlags();
  void beginMode(PbVisualMode newMode, uint32_t now, bool forceRestart = false);
  bool modeComplete(uint32_t now) const;

  PbTimingConfig config_;

  PbVisualMode visualMode_ = PbVisualMode::Off;
  uint32_t visualModeStartMs_ = 0;
  PbVerdict lastAnimatedVerdict_ = PbVerdict::Off;
  PbVerdict displayedVerdict_ = PbVerdict::Off;
  PbVerdict pendingVerdict_ = PbVerdict::Off;
  uint32_t pendingVerdictSinceMs_ = 0;
  uint16_t verdictFlipDelayMs_ = 500;
  PbVerdict stopVerdict_ = PbVerdict::Off;
  PbStopAnimStyle stopAnimStyle_ = PbStopAnimStyle::FullCollapse;
  PbVerdict wipeVerdict_ = PbVerdict::Off;

  bool startupPending_ = true;
  bool bleWakePending_ = false;
  bool bleDisconnectPending_ = false;
  bool stopAnimPending_ = false;
  bool targetAckPending_ = false;
  bool resetWipePending_ = false;
};
