# Software preparation

[Guide home](index.md)

1. Download the repository using GitHub's **Code → Download ZIP**, then extract it.
2. Open `firmware/powderbot_personality_v1_13/powderbot_personality_v1_13.ino` in Arduino IDE. Keep the sketch inside its matching folder.
3. Select **ESP32 Dev Module** and prepare the historical dependencies below.
4. Use **Verify** to compile. Save the complete build output in a bench report. Resolve missing libraries or compilation errors before considering an upload.
5. Select the actual connected board port only after identifying the board and reviewing its wiring. Historical COM6 is a record of Dan's machine.

## Recovered environment

| Item | Recorded version or setting |
| --- | --- |
| Arduino IDE | 2.3.8 |
| Arduino CLI in historical IDE log | 1.4.1 |
| ESP32 board package | 3.3.7 |
| Board | ESP32 Dev Module, `esp32:esp32:esp32` |
| EspSoftwareSerial | 8.1.0, corroborated by local library metadata |
| Adafruit NeoPixel | 1.15.4, corroborated by local library metadata |
| CPU / flash | 240 MHz / 80 MHz, 4 MB |
| Partitions / PSRAM | Default / disabled |
| Upload speed | 921600 |
| Serial Monitor | 115200 baud |

Historical build options name QIO, while generated flash arguments and packaged image headers specify DIO. Preserve this distinction when reproducing the environment. See the [cache audit](evidence/build-provenance.md).

The preserved source uses GPIO21 BLE RX, GPIO22 STATE, GPIO35 scale RX and GPIO17 strip output. It defaults to 22 pixels, brightness 100 and full animation mode. Check [behavior](behavior.md) before changing any constants.

A fresh compile and upload for this repository package have not been performed. Historical build and upload evidence is recorded separately in provenance.
