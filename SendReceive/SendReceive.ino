#include <IRremote.hpp>

const int IR_RECEIVE_PIN = 35;  // IR receiver input pin
const int IR_SEND_PIN = 25;     // IR LED output pin D4

IRData lastReceivedData;        // Stores the last received data
unsigned long lastReceiveTime = 0;
bool shouldResend = false;

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK, 18); // Start the receiver
  IrSender.begin(IR_SEND_PIN);  // Start the sender
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
    
    if (lastReceivedData.protocol == UNKNOWN) {
      // Send raw data if protocol is unknown
      IrSender.sendRaw(lastReceivedData.rawDataPtr->rawbuf + 1, lastReceivedData.rawDataPtr->rawlen - 1, 38);
    } else {
      // Send known protocol
      IrSender.write(&lastReceivedData);
    }

    shouldResend = false;
  }
}














// #include <IRremote.h>

// const int IR_SEND_PIN = 25;

// void setup() {
//   Serial.begin(115200);
//   IrSender.begin(IR_SEND_PIN);
//   Serial.println("Sending Samsung IR signal every 5 seconds...");
// }

// void loop() {
//   // Send Samsung signal: Address 0x7, Command 0x2
//   Serial.println("Sending Samsung signal: Address=0x7, Command=0x2");
//   IrSender.sendSamsung(0x7, 0x2, 32);  // 32 bits is standard for Samsung

//   delay(10);  // Wait 5 seconds before sending again

//   // TRY THIS!!
//   digitalWrite(IR_SEND_PIN, LOW);  // Turn off the emitter (IR LED)
//   delay(10);  // Wait for a brief moment
//   digitalWrite(IR_SEND_PIN, HIGH); 
// }



// #include <IRremote.hpp>

// const int IR_RECEIVE_PIN = 35;  // Pin where the IR receiver is connected

// void setup() {
//     Serial.begin(115200);   // Start serial communication
//     IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK, 18); // Start the receiver on the defined pin

//     Serial.println(F("Ready to receive IR signals"));
// }

// void loop() {
//     if (IrReceiver.decode()) {  // If an IR code is received
//         Serial.println(); // Blank line for readability
//         IrReceiver.printIRResultRawFormatted(&Serial, true);  // Print the received signal in microseconds format
//         IrReceiver.resume();  // Prepare for the next IR frame
//     }
// }
