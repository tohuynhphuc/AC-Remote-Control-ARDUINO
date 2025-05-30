#include <IRremote.hpp>

const int IR_RECEIVE_PIN = 35;

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK, 18);
  Serial.println("Waiting for IR signal...");
}

void loop() {
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.rawDataPtr != nullptr) {
      // Get raw buffer and length
      IRRawbufType* rawbuf = IrReceiver.decodedIRData.rawDataPtr->rawbuf;
      uint_fast8_t rawlen = IrReceiver.decodedIRData.rawDataPtr->rawlen;

      Serial.println("\n\nReceived IR signal! Here's the raw array:\n");

      Serial.print("{");
      for (uint_fast8_t i = 1; i < rawlen; i++) {  // Skip [0] (gap)
        Serial.print((uint16_t)(rawbuf[i] * MICROS_PER_TICK));         // Cast each value
        if (i < rawlen - 1) Serial.print(", ");
      }
      Serial.println("};");

      Serial.print("size_t rawLen = ");
      Serial.print(rawlen - 1);  // Because we skipped [0]
      Serial.println(";");

      Serial.println("\n// Use with: IrSender.sendRaw(rawData, rawLen, 38);");
    } else {
      Serial.println("No raw data available.");
    }

    IrReceiver.resume();
  }
}
