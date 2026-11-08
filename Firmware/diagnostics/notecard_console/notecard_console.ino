// notecard_console.ino - type Notecard requests in the Serial Monitor.
//
// Each line you type, a JSON request such as {"req":"card.version"}, is sent
// to the Notecard over I2C and the reply is printed. Use it to set up a new
// Notecard and to prove it reaches Notehub before loading the node firmware.
// The carrier board does not bring out the Notecard's own USB port, so this
// stands in for Blues' web terminal.
//
// Serial Monitor: 115200 baud, line ending "Newline".
// Keep the battery connected: USB alone does not power the Notecard's modem.

#include <Arduino.h>
#include <Wire.h>
#include <Notecard.h>

static const int I2C_SDA_PIN = A4;
static const int I2C_SCL_PIN = A5;
static const uint32_t I2C_CLOCK_HZ = 100000;
static const size_t MAX_LINE = 1024;

Notecard notecard;
static String line;

void setup() {
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && millis() - start < 5000) delay(10);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(I2C_CLOCK_HZ);
  notecard.begin();

  Serial.println();
  Serial.println("Notecard console ready. Type a request and press Enter, for example:");
  Serial.println("  {\"req\":\"card.version\"}");
}

static void runRequest(const String& text) {
  J* req = JParse(text.c_str());
  if (!req) {
    Serial.println("! not valid JSON - check the quotes and braces");
    return;
  }

  // A "cmd" is fire-and-forget: the Notecard sends no reply to wait for.
  const char* cmd = JGetString(req, "cmd");
  if (cmd && cmd[0]) {
    Serial.println(notecard.sendRequest(req) ? "sent (commands have no reply)"
                                             : "! the Notecard did not accept it");
    return;
  }

  J* rsp = notecard.requestAndResponse(req);
  if (!rsp) {
    Serial.println("! no reply - is the battery connected and the Notecard seated?");
    return;
  }
  char* out = JPrintUnformatted(rsp);
  Serial.println(out ? out : "! the reply could not be printed");
  if (out) JFree(out);
  notecard.deleteResponse(rsp);
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      line.trim();
      if (line.length()) {
        Serial.print("> ");
        Serial.println(line);
        runRequest(line);
      }
      line = "";
    } else if (line.length() < MAX_LINE) {
      line += c;
    }
  }
}
