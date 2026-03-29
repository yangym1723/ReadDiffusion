#include <Adafruit_MCP4728.h>
#include <Wire.h>
#include <ArduinoJson.h>

#define CV 0.819
const bool SAVE_ZERO_TO_EEPROM_ON_BOOT = false;
const int BUFFER_SIZE = 128;
char buffer[BUFFER_SIZE];
Adafruit_MCP4728 dac;
int value1, value2, value3, value4;

bool serial_output = true;

int to_dac_code(int value) {
  return constrain(int(CV * value), 0, 4095);
}

bool apply_outputs(int d1, int d2, int d3, int d4) {
  bool ok = true;
  if (d1 != 9999) {
    ok &= dac.setChannelValue(MCP4728_CHANNEL_A, to_dac_code(d1), MCP4728_VREF_VDD, MCP4728_GAIN_1X, MCP4728_PD_MODE_NORMAL);
  }
  if (d2 != 9999) {
    ok &= dac.setChannelValue(MCP4728_CHANNEL_B, to_dac_code(d2), MCP4728_VREF_VDD, MCP4728_GAIN_1X, MCP4728_PD_MODE_NORMAL);
  }
  if (d3 != 9999) {
    ok &= dac.setChannelValue(MCP4728_CHANNEL_C, to_dac_code(d3), MCP4728_VREF_VDD, MCP4728_GAIN_1X, MCP4728_PD_MODE_NORMAL);
  }
  if (d4 != 9999) {
    ok &= dac.setChannelValue(MCP4728_CHANNEL_D, to_dac_code(d4), MCP4728_VREF_VDD, MCP4728_GAIN_1X, MCP4728_PD_MODE_NORMAL);
  }
  return ok;
}

void emit_status(const char* type, const char* msg = nullptr) {
  if (!serial_output) {
    return;
  }
  StaticJsonDocument<256> doc;
  doc["type"] = type;
  if (msg != nullptr) {
    doc["msg"] = msg;
  }
  doc["d1"] = value1;
  doc["d2"] = value2;
  doc["d3"] = value3;
  doc["d4"] = value4;
  doc["dac_a"] = dac.getChannelValue(MCP4728_CHANNEL_A);
  doc["dac_b"] = dac.getChannelValue(MCP4728_CHANNEL_B);
  doc["dac_c"] = dac.getChannelValue(MCP4728_CHANNEL_C);
  doc["dac_d"] = dac.getChannelValue(MCP4728_CHANNEL_D);
  serializeJson(doc, Serial);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(200);
  Wire.begin();
  if (!dac.begin()) {
    debug("init Failed!");
  }
  value1 = 0;
  value2 = 0;
  value3 = 0;
  value4 = 0;
  if (!apply_outputs(0, 0, 0, 0)) {
    debug("boot dac write failed");
  }
  if (SAVE_ZERO_TO_EEPROM_ON_BOOT && !dac.saveToEEPROM()) {
    debug("saveToEEPROM failed");
  }
}

void loop() {
  int i = 0;
  StaticJsonDocument<256> doc;
  if (Serial.available() > 0) {
    String s = Serial.readStringUntil('\n');
    DeserializationError error = deserializeJson(doc, s);
    if (error) {
      debug("deserializeJson failed");
      return;
    }
    int cmd = doc["cmd"];
    if (cmd == 1) {
      emit_status("param");

    } else if (cmd == 2) {
      int d1 = doc["d1"];
      int d2 = doc["d2"];
      int d3 = doc["d3"];
      int d4 = doc["d4"];

      value1 = d1;
      value2 = d2;
      value3 = d3;
      value4 = d4;
      if (!apply_outputs(d1, d2, d3, d4)) {
        emit_status("error", "dac write failed");
        return;
      }
      emit_status("done");

    } else if (cmd == 3) {
      if (!dac.saveToEEPROM()) {
        emit_status("error", "saveToEEPROM failed");
        return;
      }
      emit_status("saved", "eeprom updated");

    } else {
      Serial.println("Error parsing JSON data");
      return;
    }
  }
}

void debug(int value) {
  if (!serial_output) {
    return;
  }
  StaticJsonDocument<50> param;
  param["type"] = "debug";
  param["value"] = value;
  serializeJson(param, Serial);
}
void debug(String msg) {
  if (!serial_output) {
    return;
  }
  StaticJsonDocument<50> param;
  param["type"] = "debug";
  param["msg"] = msg;
  serializeJson(param, Serial);
}

void debug(String msg, int value) {
  if (!serial_output) {
    return;
  }
  StaticJsonDocument<50> param;
  param["type"] = "debug";
  param["msg"] = msg;
  param["value"] = value;
  serializeJson(param, Serial);
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  float value = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  value = constrain(value, out_min, out_max);
  return value;
}
