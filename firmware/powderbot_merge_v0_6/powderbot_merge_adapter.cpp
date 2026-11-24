#include "powderbot_merge_adapter.h"

#include <Arduino.h>


using namespace PowderBotProtocol;

PowderBotMergeAdapter::PowderBotMergeAdapter(PowderBotEngine& engine,
                                             PowderBotPresentation& presentation)
  : PowderBotMergeAdapter(engine, presentation, TraceConfig()) {
}

PowderBotMergeAdapter::PowderBotMergeAdapter(PowderBotEngine& engine,
                                             PowderBotPresentation& presentation,
                                             const TraceConfig& traceConfig)
  : engine_(engine), presentation_(presentation), traceConfig_(traceConfig) {
}

void PowderBotMergeAdapter::syncEngineEvents() {
  const PbPendingEvents events = engine_.consumePendingEvents();
  const PbEngineSnapshot snapshot = engine_.snapshot();
  presentation_.applyEngineEvents(events, snapshot, millis());
}

void PowderBotMergeAdapter::onBleConnectionChanged(bool connected) {
  engine_.onBleConnectionChanged(connected);
  syncEngineEvents();
}

void PowderBotMergeAdapter::logUnhandledCommand(const PbCommand& command) {
  if (!traceConfig_.unhandledCommands) return;
  Serial.print("MERGE ADAPTER: command observed but not owned here yet -> ");
  Serial.print(pbCommandTypeToString(command.type));
  if (command.text[0] != '\0') {
    Serial.print(" text=");
    Serial.print(command.text);
  }
  Serial.println();
}

void PowderBotMergeAdapter::onCommandReceived(const PbCommand& command) {
  switch (command.type) {
    case PB_CMD_SET_TARGET_GN:
      engine_.onTargetReceived(command.valueGn);
      syncEngineEvents();
      break;

    case PB_CMD_STOP:
      engine_.onStopCommand();
      syncEngineEvents();
      break;

    case PB_CMD_REQUEST_STATUS:
    case PB_CMD_REQUEST_PARAMS:
    case PB_CMD_REQUEST_MACHINE_SNAPSHOT:
    case PB_CMD_RUN_THROW:
    case PB_CMD_RUN_TRICKLER:
    case PB_CMD_STOP_TRICKLER:
    case PB_CMD_THROW_ON:
    case PB_CMD_THROW_OFF:
    case PB_CMD_SCALE_REQUEST_IMMEDIATE:
    case PB_CMD_SCALE_REQUEST_STABLE:
    case PB_CMD_SCALE_STREAM_ON:
    case PB_CMD_SCALE_STREAM_OFF:
    case PB_CMD_SCALE_CANCEL:
    case PB_CMD_SCALE_TARE:
    case PB_CMD_SCALE_ZERO:
    case PB_CMD_LEGACY_TEXT:
    case PB_CMD_DEBUG_NOTE:
    case PB_CMD_CLEAR_TARGET:
    case PB_CMD_ARM:
    case PB_CMD_DISARM:
    case PB_CMD_NONE:
    default:
      logUnhandledCommand(command);
      break;
  }
}

void PowderBotMergeAdapter::onScaleSampleReceived(const PbScaleSample& sample) {
  if (!sample.valid) return;
  engine_.onScaleSample(sample.stable, sample.weightGn);
  syncEngineEvents();
}

void PowderBotMergeAdapter::onRawTrafficLine(const char* channel, const char* line) {
  if (!traceConfig_.rawTraffic || !channel || !line) return;

  Serial.print("TRACE[");
  Serial.print(channel);
  Serial.print("]: ");
  Serial.println(line);
}
