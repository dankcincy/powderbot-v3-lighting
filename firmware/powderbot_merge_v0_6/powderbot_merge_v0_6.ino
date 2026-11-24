/*
      PowderBot 9000 - powderbot_merge_v0_6

      First merge skeleton:
      - powderbot_comms_v1_2 remains the transport owner
      - powderbot_engine / presentation / renderer remain the machine and visual owner
      - powderbot_merge_adapter is the thin seam between them

      This is intentionally not the final machine-control merge yet.
      It preserves the known-good BT05/V3 bridge while letting BLE and scale events
      drive the PowderBot core visual/machine logic.

      USB serial commands:
      - vf / full         -> full show
      - vb / boomer       -> boomer mode
      - vs / show         -> print current show mode
      - vh / help         -> print serial command list
      - vt / tol          -> print current under/over tolerances = vt or tol
      - tu <value>        -> set under tolerance in grains = tu .02
      - to <value>        -> set over tolerance in grains = to .02
      - tb <u> <o>        -> set both tolerances in grains = tb .02 .02
      - vd / vdelay       -> print visual verdict flip delay in ms = vd
      - vd <ms>           -> set visual verdict flip delay in ms = vd 500
      - vv / vstate       -> print visual verdict filter state = vv

      Default verdict tolerances still live in powderbot_merge_config.h:
      - UNDER_TOLERANCE_GN
      - OVER_TOLERANCE_GN
    */

    #include <Adafruit_NeoPixel.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

    #include "powderbot_comms_v1_2.h"
    #include "powderbot_engine.h"
    #include "powderbot_merge_adapter.h"
    #include "powderbot_merge_config.h"
    #include "powderbot_presentation.h"
    #include "powderbot_strip_renderer.h"

    namespace {
      Adafruit_NeoPixel g_strip(PowderBotMergeConfig::NUM_PIXELS,
                                PowderBotMergeConfig::STRIP_DATA_PIN,
                                NEO_GRB + NEO_KHZ800);

      PowderBotEngine g_engine(PowderBotMergeConfig::ENGINE_CONFIG);
      PowderBotPresentation g_presentation(PowderBotMergeConfig::TIMING_CONFIG);
      PowderBotStripRenderer g_renderer(g_strip,
                                        PowderBotMergeConfig::NUM_PIXELS,
                                        PowderBotMergeConfig::TIMING_CONFIG);
      PowderBotMergeAdapter g_adapter(
          g_engine,
          g_presentation,
          {PowderBotMergeConfig::TRACE_RAW_TRAFFIC,
           PowderBotMergeConfig::TRACE_UNHANDLED_COMMANDS});

      PbCommsHubConfig g_commsConfig = pbCommsDefaultConfig();
      PbCommsEventHandlers g_commsHandlers;

      void onBleConnectionChanged(bool connected) {
        g_adapter.onBleConnectionChanged(connected);
      }

      void onCommandReceived(const PowderBotProtocol::PbCommand& command) {
        g_adapter.onCommandReceived(command);
      }

      void onScaleSampleReceived(const PowderBotProtocol::PbScaleSample& sample) {
        g_adapter.onScaleSampleReceived(sample);
      }

      void onRawTrafficLine(const char* channel, const char* line) {
                g_adapter.onRawTrafficLine(channel, line);
      }


      void configureCommsHub() {
        // Known-good baseline identity.
        g_commsConfig.bleName = "BT05";

        // Keep the bridge behavior visible while the merge settles in, but take the
        // actual tap/debug defaults from merge config instead of hardwiring them here.
        g_commsConfig.trafficTapEnabled = PowderBotMergeConfig::COMMS_TRAFFIC_TAP_ENABLED;
        g_commsConfig.tapAppToV3 = PowderBotMergeConfig::COMMS_TAP_APP_TO_V3;
        g_commsConfig.tapV3ToApp = PowderBotMergeConfig::COMMS_TAP_V3_TO_APP;
        g_commsConfig.tapScaleMain = PowderBotMergeConfig::COMMS_TAP_SCALE_MAIN;
        g_commsConfig.tapScaleCmd = PowderBotMergeConfig::COMMS_TAP_SCALE_CMD;
        g_commsConfig.tapScaleSummary = PowderBotMergeConfig::COMMS_TAP_SCALE_SUMMARY;

        // Locked Bart's-board comms mapping.
        g_commsConfig.v3RxPin = 34;          // V3 TX -> GPIO34
        g_commsConfig.v3TxPin = 22;          // V3 RX <- GPIO22
        g_commsConfig.scaleLinkMode = PB_SCALE_LINK_PASSIVE_ONLY;
        g_commsConfig.scaleRxPin = 32;       // MAX3232 R1 OUT -> GPIO32
        g_commsConfig.scaleTxPin = -1;
        g_commsConfig.scaleCmdTapRxPin = 33; // MAX3232 R2 OUT -> GPIO33
        g_commsConfig.scaleBaud = 19200;
        g_commsConfig.scaleSerialConfig = SERIAL_8N1;
        g_commsConfig.scaleCmdTerminator = "\r\n";
        g_commsConfig.dropV3TrafficWhenBleDisconnected = true;
      }

      void printToleranceState() {
        Serial.print("TOLERANCES -> under=");
        Serial.print(g_engine.underToleranceGn(), 3);
        Serial.print(" gn, over=");
        Serial.print(g_engine.overToleranceGn(), 3);
        Serial.println(" gn");
      }

      void printVisualVerdictFilterState() {
        Serial.print("VISUAL VERDICT FILTER -> delay=");
        Serial.print(g_presentation.verdictFlipDelayMs());
        Serial.print(" ms, displayed=");
        Serial.print(pbVerdictName(g_presentation.displayedVerdict()));
        Serial.print(", pending=");
        Serial.println(pbVerdictName(g_presentation.pendingVerdict()));
      }

      void printShowModeHelp() {
        Serial.println("Serial commands:");
        Serial.println("  vf / full        -> full show");
        Serial.println("  vb / boomer      -> boomer mode");
        Serial.println("  vs / show        -> print current mode");
        Serial.println("  vt / tol         -> print current under/over tolerances");
        Serial.println("  tu <value>       -> set under tolerance in grains");
        Serial.println("  to <value>       -> set over tolerance in grains");
        Serial.println("  tb <u> <o>       -> set both tolerances in grains");
        Serial.println("  vd / vdelay      -> print visual verdict flip delay");
        Serial.println("  vd <ms>          -> set visual verdict flip delay in ms");
        Serial.println("  vv / vstate      -> print visual verdict filter state");
        Serial.println("  vh / help        -> print this command list");
      }

      void printShowModeState() {
        Serial.print("SHOW MODE -> ");
        Serial.println(g_presentation.fullShowEnabled() ? "FULL SHOW" : "BOOMER MODE");
      }

      void setShowMode(bool fullShow) {
        if (g_presentation.fullShowEnabled() == fullShow) {
          printShowModeState();
          return;
        }

        g_presentation.setFullShow(fullShow);
        Serial.print("SHOW MODE SET -> ");
        Serial.println(fullShow ? "FULL SHOW" : "BOOMER MODE");
      }

      void setUnderTolerance(float valueGn) {
        g_engine.setUnderToleranceGn(valueGn);
        Serial.print("UNDER TOLERANCE SET -> ");
        Serial.print(g_engine.underToleranceGn(), 3);
        Serial.println(" gn");
        printToleranceState();
      }

      void setOverTolerance(float valueGn) {
        g_engine.setOverToleranceGn(valueGn);
        Serial.print("OVER TOLERANCE SET -> ");
        Serial.print(g_engine.overToleranceGn(), 3);
        Serial.println(" gn");
        printToleranceState();
      }

      void setBothTolerances(float underGn, float overGn) {
        g_engine.setVerdictTolerancesGn(underGn, overGn);
        Serial.println("BOTH TOLERANCES UPDATED");
        printToleranceState();
      }

      void setVisualVerdictFlipDelayMs(uint16_t delayMs) {
        g_presentation.setVerdictFlipDelayMs(delayMs);
        Serial.print("VISUAL VERDICT FLIP DELAY SET -> ");
        Serial.print(g_presentation.verdictFlipDelayMs());
        Serial.println(" ms");
        printVisualVerdictFilterState();
      }

      void trimWhitespaceInPlace(char* text) {
        if (!text) return;
        size_t len = strlen(text);
        while (len > 0 && isspace((unsigned char)text[len - 1])) {
          text[--len] = '\0';
        }
        size_t start = 0;
        while (text[start] && isspace((unsigned char)text[start])) {
          ++start;
        }
        if (start > 0) {
          memmove(text, text + start, strlen(text + start) + 1);
        }
      }

      void handleLocalSerialCommand(const char* line) {
        if (!line || !line[0]) return;

        if (strcmp(line, "vf") == 0 || strcmp(line, "full") == 0 || strcmp(line, "fullshow") == 0) {
          setShowMode(true);
          return;
        }

        if (strcmp(line, "vb") == 0 || strcmp(line, "boomer") == 0 || strcmp(line, "boom") == 0) {
          setShowMode(false);
          return;
        }

        if (strcmp(line, "vs") == 0 || strcmp(line, "show") == 0) {
          printShowModeState();
          return;
        }

        if (strcmp(line, "vt") == 0 || strcmp(line, "tol") == 0 || strcmp(line, "tols") == 0) {
          printToleranceState();
          return;
        }

        if (strcmp(line, "vd") == 0 || strcmp(line, "vdelay") == 0) {
          printVisualVerdictFilterState();
          return;
        }

        if (strcmp(line, "vv") == 0 || strcmp(line, "vstate") == 0) {
          printVisualVerdictFilterState();
          return;
        }

        float valueA = 0.0f;
        float valueB = 0.0f;
        unsigned int valueMs = 0;
        if (sscanf(line, "tu %f", &valueA) == 1) {
          setUnderTolerance(valueA);
          return;
        }

        if (sscanf(line, "to %f", &valueA) == 1) {
          setOverTolerance(valueA);
          return;
        }

        if (sscanf(line, "tb %f %f", &valueA, &valueB) == 2) {
          setBothTolerances(valueA, valueB);
          return;
        }

        if (sscanf(line, "vd %u", &valueMs) == 1) {
          setVisualVerdictFlipDelayMs((uint16_t)valueMs);
          return;
        }

        if (strcmp(line, "vh") == 0 || strcmp(line, "help") == 0) {
          printShowModeHelp();
          return;
        }

        if (pbCommsHandleConsoleCommand(line, Serial)) {
          return;
        }

        Serial.print("Unknown serial command: ");
        Serial.println(line);
        printShowModeHelp();
      }

      void pollLocalSerialCommands() {
        static char cmdBuf[48];
        static size_t cmdLen = 0;

        while (Serial.available() > 0) {
          const int raw = Serial.read();
          if (raw < 0) break;
          const char c = (char)raw;
          if (c == '\r') continue;
          if (c == '\n') {
            cmdBuf[cmdLen] = '\0';
            if (cmdLen > 0) {
              trimWhitespaceInPlace(cmdBuf);
              for (size_t i = 0; i < strlen(cmdBuf); ++i) {
                cmdBuf[i] = (char)tolower((unsigned char)cmdBuf[i]);
              }
              handleLocalSerialCommand(cmdBuf);
            }
            cmdLen = 0;
            continue;
          }
          if (cmdLen < sizeof(cmdBuf) - 1) {
            cmdBuf[cmdLen++] = c;
          }
        }
      }

      void configureHandlers() {
        g_commsHandlers.onBleConnectionChanged = onBleConnectionChanged;
        g_commsHandlers.onCommandReceived = onCommandReceived;
        g_commsHandlers.onScaleSampleReceived = onScaleSampleReceived;
        g_commsHandlers.onRawTrafficLine = onRawTrafficLine;
      }
    }  // namespace

    void setup() {
      Serial.begin(PowderBotMergeConfig::USB_BAUD);
      delay(250);

      g_renderer.begin(PowderBotMergeConfig::LED_BRIGHTNESS);

      g_engine.reset(false);
      g_presentation.reset(false, millis());
      g_presentation.setVerdictFlipDelayMs(PowderBotMergeConfig::VISUAL_VERDICT_FLIP_DELAY_MS);
      g_adapter.syncEngineEvents();

      configureCommsHub();
      configureHandlers();
      pbCommsSetEventHandlers(g_commsHandlers);
      pbCommsBegin(g_commsConfig);

      Serial.println();
      Serial.println("PowderBot 9000 - powderbot_merge_v0_6");
      Serial.println("--------------------------------------------------");
      Serial.println("Ownership:");
      Serial.println("  comms hub      = BLE / V3 / scale transport + event emission");
      Serial.println("  merge adapter  = transport-to-engine seam");
      Serial.println("  engine         = machine truth / target / verdict / reset");
      Serial.println("  presentation   = visual policy / one-shot transitions");
      Serial.println("  renderer       = strip-only drawing");
      Serial.println();
      Serial.println("This build keeps the known-good bridge alive while core visuals ride on top.");
      printShowModeHelp();
      printShowModeState();
      printToleranceState();
      printVisualVerdictFilterState();
    }

    void loop() {
      pollLocalSerialCommands();
      pbCommsUpdate();

      const PbEngineSnapshot snapshot = g_engine.snapshot();
      g_presentation.update(snapshot, millis());
      g_renderer.render(g_presentation.renderState(), millis());
    }