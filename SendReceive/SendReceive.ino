#include <IRremote.hpp>

const int IR_RECEIVE_PIN = 35;  // IR receiver input pin A0
const int IR_SEND_PIN = 25;     // IR LED output pin D4

IRData lastReceivedData;  // Stores the last received data
unsigned long lastReceiveTime = 0;
bool shouldResend = false;

void setup() {
    Serial.begin(115200);
    IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK, 18);
    IrSender.begin(IR_SEND_PIN);
    pinMode(IR_SEND_PIN, OUTPUT);
    Serial.println("Ready to receive IR signal and resend after 5 seconds");
}

void loop() {
    // Check for new IR signal
    if (IrReceiver.decode()) {
        Serial.println("\nIR signal received:");
        IrReceiver.printIRResultRawFormatted(&Serial, true);

        // Save received data for sending later
        lastReceivedData = IrReceiver.decodedIRData;
        lastReceiveTime = millis();
        shouldResend = true;

        IrReceiver.resume();  // Ready to receive next signal
    }

    // If it's time to resend
    if (shouldResend && millis() - lastReceiveTime >= 5000) {
        Serial.println("Resending received IR signal...");
        // Send raw data if protocol is unknown
        IrSender.sendRaw(lastReceivedData.rawDataPtr->rawbuf + 1,
                         lastReceivedData.rawDataPtr->rawlen - 1, 38);

        shouldResend = false;
        delay(100);
    }
}