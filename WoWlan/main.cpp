#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

typedef uint8_t MAC[6];

// --- Configuration
#if __has_include("WIFI.h")
#include "WIFI.h"
#else
const char* ssid = "Your SSID";
const char* password = "Your Password";
#endif

MAC MAC_ADDRESS = {0xD8, 0x3B, 0xBF, 0xE5, 0x6C, 0xD0};

#define WOL_PORT 9
// --- End Configuration

void setupPowerOn();
void powerOn();
bool isWolPacket(const uint8_t*, size_t, MAC);

#ifdef DEBUG
void printBytes(const uint8_t*, size_t);
#endif

WiFiUDP listen = WiFiUDP();

#define WAKE_PACKET_LENGTH 102
uint8_t packetBuffer[WAKE_PACKET_LENGTH];

void setup() {
#ifdef DEBUG
    // For debugging
    Serial.begin(9600);
#endif

    // Initialize built in led
    setupPowerOn();

    // Setup Wifi
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

#ifdef DEBUG
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.print(WiFi.localIP());
    Serial.println();
#endif

    if (!listen.begin(WOL_PORT)) {
#ifdef DEBUG
        Serial.println("Failed bind UDP Socket: Restarting...");
#endif
        delay(1000);
        ESP.restart();
    }
}

void loop() {
    int packetSize = listen.parsePacket();
    if (packetSize) {
#ifdef DEBUG
        Serial.print("Received Packet: ");
#endif

        int read = listen.readBytes(packetBuffer, sizeof(packetBuffer));

        if (isWolPacket(packetBuffer, read, MAC_ADDRESS)) {
            powerOn();
        } else {
#ifdef DEBUG
            Serial.print("Not Matching: ");
            Serial.println(read);

            Serial.print("Bytes: ");
            printBytes(packetBuffer, packetSize);
            Serial.println();
#endif
        }
    }
}

bool isWolPacket(const uint8_t* buffer, size_t bufferSize, MAC mac) {
    if (bufferSize != 102) {
        return false;
    }

    const uint8_t WOL_START[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (memcmp(buffer, WOL_START, 6) != 0) {
        return false;
    }
#ifdef DEBUG
    Serial.print("Magic Packet, ");
#endif

    for (int i = 0; i < 16; i++) {
        int index = (i + 1) * 6;

#ifdef DEBUG
        Serial.print(i, 16);
#endif

        if (memcmp(&buffer[index], MAC_ADDRESS, 6) != 0) {
            Serial.print(", ");
            return false;
        }
    }

#ifdef DEBUG
    Serial.print(", ");
#endif

    return true;
}

void setupPowerOn() {
    pinMode(D0, OUTPUT);
    digitalWrite(D0, HIGH);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
}
void powerOn() {
#ifdef DEBUG
    Serial.println("Wake");
#endif

    digitalWrite(D0, HIGH);
    digitalWrite(LED_BUILTIN, LOW);

    delay(1000);

    digitalWrite(D0, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
}

#ifdef DEBUG
void printBytes(const uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        Serial.print(buffer[i], HEX);
    }
}
#endif