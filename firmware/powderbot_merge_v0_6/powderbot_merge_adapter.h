#pragma once

#include "powderbot_comms_protocol.h"
#include "powderbot_engine.h"
#include "powderbot_presentation.h"

class PowderBotMergeAdapter {
public:
  struct TraceConfig {
    bool rawTraffic;
    bool unhandledCommands;

    TraceConfig() : rawTraffic(true), unhandledCommands(true) {}
    TraceConfig(bool rawTrafficEnabled, bool unhandledCommandsEnabled)
      : rawTraffic(rawTrafficEnabled), unhandledCommands(unhandledCommandsEnabled) {}
  };

  PowderBotMergeAdapter(PowderBotEngine& engine, PowderBotPresentation& presentation);
  PowderBotMergeAdapter(PowderBotEngine& engine, PowderBotPresentation& presentation, const TraceConfig& traceConfig);

  void syncEngineEvents();
  void onBleConnectionChanged(bool connected);
  void onCommandReceived(const PowderBotProtocol::PbCommand& command);
  void onScaleSampleReceived(const PowderBotProtocol::PbScaleSample& sample);
  void onRawTrafficLine(const char* channel, const char* line);

private:
  void logUnhandledCommand(const PowderBotProtocol::PbCommand& command);

  PowderBotEngine& engine_;
  PowderBotPresentation& presentation_;
  TraceConfig traceConfig_;
};
