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

const byte LI_VN_MODE = 0b00100100; 
const byte STRATUM    = 2;          
const byte POLL       = 6;         
const byte PRECISION  = 0xEC;       

byte referenceId[4] = {0, 0, 0, 0};

#define DEMO_FAKE_TIME true
const uint32_t FAKE_UNIX_TIME = 1893456000UL; // 1 enero 2030 

void writeTimestamp(int startIndex, uint32_t seconds); 

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

    IPAddress sourceIP;
    if (WiFi.hostByName(ntpServer, sourceIP)) {
      referenceId[0] = sourceIP[0];
      referenceId[1] = sourceIP[1];
      referenceId[2] = sourceIP[2];
      referenceId[3] = sourceIP[3];
    }

    WiFi.softAP("Server Test");
    WiFi.softAPConfig(local_IP, gateway, subnet);

    ntpUDP.begin(NTP_PORT);
    Serial.println("Servidor NTP escuchando en puerto 123");

    if (DEMO_FAKE_TIME) {
      Serial.println("*** MODO DEMO ACTIVO: sirviendo hora falsa a los clientes ***");
    }
}

void loop(){
    int packetSize = ntpUDP.parsePacket();
    if (packetSize) {
        Serial.printf("Pedido NTP recibido, %d bytes\n", packetSize);

        ntpUDP.read(packetBuffer, 48);

        byte clientTransmitTime[8];
        memcpy(clientTransmitTime, packetBuffer + 40, 8);

        time_t recvTime;
        time(&recvTime);
        uint32_t ntpRecvTime = DEMO_FAKE_TIME
            ? (FAKE_UNIX_TIME + NTP_UNIX_EPOCH_DIFF)
            : ((uint32_t)recvTime + NTP_UNIX_EPOCH_DIFF);

        memset(packetBuffer, 0, 48);

        packetBuffer[0] = LI_VN_MODE;
        packetBuffer[1] = STRATUM;
        packetBuffer[2] = POLL;
        packetBuffer[3] = PRECISION;

        memcpy(packetBuffer + 12, referenceId, 4);

        writeTimestamp(16, ntpRecvTime);

        memcpy(packetBuffer + 24, clientTransmitTime, 8);

        writeTimestamp(32, ntpRecvTime);

        time_t sendTime;
        time(&sendTime);
        uint32_t ntpSendTime = DEMO_FAKE_TIME
            ? (FAKE_UNIX_TIME + NTP_UNIX_EPOCH_DIFF)
            : ((uint32_t)sendTime + NTP_UNIX_EPOCH_DIFF);
        writeTimestamp(40, ntpSendTime);

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
