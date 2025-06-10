#include <HTTPClient.h>
#include <Preferences.h>
#include <WebSocketsClient.h>
#include <WiFi.h>

#include <IRremote.hpp>

#include "Password.h"      // Contains USERNAME and PASSWORD for login
#include "SavedSignals.h"  // Contains IR signal data

// ---------- WiFi Credentials ----------
const char* ssid = "Lord Voldemodem";

const char* uni_ssid = "VGU_Student_Guest";
const char* uni_password = "";

// Tracking connected WiFi
enum WifiType { NONE, HOME, UNI };
WifiType currentWiFi = NONE;

// Timers
unsigned long lastHomeRetryTime = 0;
const unsigned long HOME_RETRY_INTERVAL = 60000;
unsigned long lastMessageTime = 0;

// ---------- Actual Timer ----------
int timer = 0;
bool isTimer = false;
unsigned long timerEndTime = 0;
Preferences preferences;

// ---------- Pins ----------
const int IR_SEND_PIN = 25;  // D4
const int LED_PIN = 18;      // BUILT-IN LED

// ---------- WebSocket Settings ----------
const char* websockets_server_host = "107.175.3.39";
const uint16_t websockets_server_port = 5179;
const char* websockets_server_path = "/ws";

// ---------- Login URL ----------
const String login_url = "https://smartac.20050703.xyz/login";

WebSocketsClient client;
String sessionCookie = "";

// ---------- WebSocket Event Handler ----------
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("[WSc] Disconnected");
            break;
        case WStype_CONNECTED:
            Serial.println("[WSc] Connected to server");
            break;
        case WStype_TEXT: {
            // Received a signal
            Serial.print("[WSc] Received: ");
            Serial.println((char*)payload);
            lastMessageTime = millis();

            // Blink LED to signify received signal
            digitalWrite(LED_PIN, HIGH);
            delay(50);
            digitalWrite(LED_PIN, LOW);
            delay(50);
            digitalWrite(LED_PIN, HIGH);

            String signal = String((char*)payload);

            // Check for timer suffix
            int lastUnderscore = signal.lastIndexOf('_');
            if (lastUnderscore != -1 && lastUnderscore < signal.length() - 1) {
                String possibleNumber = signal.substring(lastUnderscore + 1);
                bool isNumber = true;

                // Check if the substring is a number
                for (unsigned int j = 0; j < possibleNumber.length(); j++) {
                    if (!isDigit(possibleNumber.charAt(j))) {
                        isNumber = false;
                        break;
                    }
                }

                if (isNumber) {
                    timer = possibleNumber.toInt();
                    isTimer = true;
                    timerEndTime = millis() + (unsigned long)timer * 60000;
                    saveTimerInfo();
                    signal = signal.substring(0, lastUnderscore);
                } else {
                    timer = 0;
                    isTimer = false;
                    timerEndTime = 0;
                    saveTimerInfo();
                }
            }

            Serial.println("Extracted Signal: " + signal);

            // Check if it is a known signal and send it
            for (int i = 0; i < SIGNALS_COUNT; i++) {
                if (signal == savedSignals[i].name) {
                    IrSender.sendRaw(savedSignals[i].data, IR_SIGNAL_LENGTH,
                                     FREQUENCY);
                    Serial.println("IR Signal Sent: " + signal);
                    break;
                }
            }

            break;
        }
        case WStype_BIN:
            Serial.println("[WSc] Received binary data (not shown)");
            break;
        case WStype_PING:
            Serial.println("[WSc] Received ping");
            break;
        case WStype_PONG:
            Serial.println("[WSc] Received pong");
            break;
        default:
            Serial.println("[WSc] Unknown WebSocket event");
            break;
    }
}

// ---------- Connect to any available WiFi ----------
void connectToWiFi() {
    Serial.println("Trying to connect to Home WiFi...");
    WiFi.begin(ssid, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Home WiFi.");
        currentWiFi = HOME;
        return;
    }

    Serial.println("\nHome WiFi failed. Trying Uni WiFi...");
    WiFi.begin(uni_ssid, uni_password);

    start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Uni WiFi.");
        currentWiFi = UNI;
    } else {
        Serial.println("\nFailed to connect to any WiFi.");
        currentWiFi = NONE;
    }
}

// ---------- Periodically reconnects to Home WiFi ----------
void switchToHomeWiFiIfAvailable() {
    if (millis() - lastHomeRetryTime < HOME_RETRY_INTERVAL) return;
    lastHomeRetryTime = millis();

    Serial.println("Checking if Home WiFi is available...");
    WiFi.begin(ssid, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 3000) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nSwitched back to Home WiFi.");
        currentWiFi = HOME;
    } else {
        Serial.println("\nHome WiFi still unavailable. Reconnecting to Uni...");
        WiFi.begin(uni_ssid, uni_password);
        currentWiFi = UNI;
    }
}

// ---------- Log in to the Server and get Session Cookie ----------
void loginToServer() {
    Serial.println("Attempting login...");

    HTTPClient http;
    http.begin(login_url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    const char* headerKeys[] = {"Set-Cookie"};
    const size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
    http.collectHeaders(headerKeys, headerKeysCount);

    String postData = "username=" + USERNAME + "&password=" + PASSWORD;
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Login succeed, response code: " +
                       String(httpResponseCode));
        Serial.println("Response body: " + response);

        if (http.hasHeader("Set-Cookie")) {
            sessionCookie = http.header("Set-Cookie");
            Serial.println("Session Cookie: " + sessionCookie);
        } else {
            Serial.println("No Set-Cookie header found.");
        }
    } else {
        Serial.println("Login failed, error: " + String(httpResponseCode));
    }

    http.end();
}

// ---------- Setup WebSocket connection ----------
void setupWebSocket() {
    // Setup WebSocket connection
    client.begin(websockets_server_host, websockets_server_port,
                 websockets_server_path);

    // Send cookie in the header
    client.setExtraHeaders((String("Cookie: ") + sessionCookie).c_str());
    client.onEvent(webSocketEvent);

    // Try to reconnect every 5s if disconnected
    client.setReconnectInterval(5000);
}

// ---------- Update built-in LED ----------
void updateLED() {
    if (WiFi.status() == WL_CONNECTED && client.isConnected()) {
        digitalWrite(LED_PIN, HIGH);
    } else {
        digitalWrite(LED_PIN, LOW);
    }
}

// ---------- Save/Load Timer Information ----------
void saveTimerInfo() {
    preferences.begin("timer", false);
    preferences.putBool("isTimer", isTimer);
    preferences.putULong("timerEndTime", timerEndTime);
    preferences.putInt("timer", timer);
    preferences.end();
}

void loadTimerInfo() {
    preferences.begin("timer", true);  // read-only
    isTimer = preferences.getBool("isTimer", false);
    timerEndTime = preferences.getULong("timerEndTime", 0);
    timer = preferences.getInt("timer", 0);
    preferences.end();

    if (isTimer) {
        Serial.println("Loaded timer from storage:");
        Serial.println("Timer active, ends at: " + String(timerEndTime));
        Serial.println("Timer duration (minutes): " + String(timer));
    } else {
        Serial.println("No active timer loaded.");
    }
}

// ---------- Arduino Setup (Run once) ----------
void setup() {
    Serial.begin(115200);
    IrSender.begin(IR_SEND_PIN);
    pinMode(LED_PIN, OUTPUT);

    loadTimerInfo();
    connectToWiFi();
    loginToServer();
    setupWebSocket();
}

// ---------- Arduino Main Loop ----------
void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected. Reconnecting...");
        connectToWiFi();
        delay(1000);
        return;
    }

    // If currently on Uni WiFi, check if Home is available
    if (currentWiFi == UNI) {
        switchToHomeWiFiIfAvailable();
    }

    // Check for signals frequently
    client.loop();
    updateLED();

    // Timer countdown check
    if (isTimer && millis() >= timerEndTime) {
        Serial.println("Timer expired. Sending OFF signal...");

        // Find and send the OFF signal
        for (int i = 0; i < SIGNALS_COUNT; i++) {
            if (savedSignals[i].name == OFF) {
                IrSender.sendRaw(savedSignals[i].data, IR_SIGNAL_LENGTH,
                                 FREQUENCY);
                Serial.println("IR Signal Sent: OFF");
                break;
            }
        }

        isTimer = false;
        timer = 0;
        timerEndTime = 0;
    }

    // Restart the ESP every 5 minutes
    if (millis() - lastMessageTime > 300000) {  // 5 minutes
        ESP.restart();
    }
}