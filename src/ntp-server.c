#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "time.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800;
const int   daylightOffset_sec = 0;

IPAddress local_IP(192,168,1,2);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

const uint32_t NTP_UNIX_EPOCH_DIFF = 2208988800UL;

WiFiUDP ntpUDP;
const unsigned int NTP_PORT = 123;
byte packetBuffer[48];

void setup(){
    Serial.begin(115200);
    Serial.println();

    WiFi.mode(WIFI_AP_STA);

    WiFi.begin("ssid", "password");

    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    Serial.println("Esperando sincronizacion NTP...");
    while (!getLocalTime(&timeinfo)) {
      Serial.print(".");
      delay(500);
    }
    char buffer[40];
    strftime(buffer, sizeof(buffer), "%A, %d %B %Y %H:%M:%S", &timeinfo);
    Serial.println();
    Serial.println(buffer);

    WiFi.softAP("Server Test");
    WiFi.softAPConfig(local_IP, gateway, subnet);

    ntpUDP.begin(NTP_PORT);
    Serial.println("Servidor NTP escuchando en puerto 123");
}

void loop(){
    int packetSize = ntpUDP.parsePacket();
    if (packetSize) {
        Serial.printf("Pedido NTP recibido, %d bytes\n", packetSize);

        ntpUDP.read(packetBuffer, 48);

        byte clientTransmitTime[0];
        memcpy(clientTransmitTime, packetBuffer + 40, 8);

        memset(packetBuffer, 0, 48);

        packetBuffer[0] = 0b00100100;

        packetBuffer[1] = 1;   
        packetBuffer[2] = 6;   
        packetBuffer[3] = 0xEC; 

        time_t now;
        time(&now);

        uint32_t ntpTime = (uint32_t)now + NTP_UNIX_EPOCH_DIFF;

        writeTimestamp(16, ntpTime);

        memcpy(packetBuffer + 24, clientTransmitTime, 8);

        writeTimestamp(32, ntpTime);

        writeTimestamp(40, ntpTime);

        ntpUDP.beginPacket(ntpUDP.remoteIP(), ntpUDP.remotePort());
        ntpUDP.write(packetBuffer, 48);
        ntpUDP.endPacket();

        Serial.println("Respuesta NTP enviada");
    }

    delay(10);
}


void writeTimestamp(int startIndex, uint32_t seconds) {
    packetBuffer[startIndex]     = (seconds >> 24) & 0xFF;
    packetBuffer[startIndex + 1] = (seconds >> 16) & 0xFF;
    packetBuffer[startIndex + 2] = (seconds >> 8) & 0xFF;
    packetBuffer[startIndex + 3] = seconds & 0xFF;
}
