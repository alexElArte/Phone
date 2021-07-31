/*
  List:
  - search update                | OK
  - get update                   | OK
  - simple tcp client connection | OK
  - auto update (si possible)    | BAD
  - wifi scan                    | OK
  - wifi connection              | OK
  - wifi get current time        | OK
  - wifi boot/shutdown           |
  - wifi internet connect        | OK
  - wifi station connect         | OK

*/
#define VERSION_STR "1.0"
#define VERSION_INT 0x10

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>

ESP8266WiFiMulti WiFiMulti;

#include "code.h"


uint32_t _prev = 0;
uint16_t _inter = 2000;

void setup() {
  Serial.begin(1000000);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  WiFi.mode(WIFI_STA);

  WiFiMulti.addAP("SFR-4a30", "QEILEDE2UQ37");
  WiFiMulti.addAP("SFR-4a31", "QEILEDE2UQ37");
  WiFiMulti.addAP("Livebox-EC87", "2206E9427F1C6EE53CEE5322AF");
}


void loop() {
  // Update wifi connection
  if (millis() > (_inter + _prev)) {
    if(WiFiMulti.run() == WL_CONNECTED) digitalWrite(LED_BUILTIN, LOW);
    else digitalWrite(LED_BUILTIN, HIGH);
  }

  if (Serial.available()) {
    uint8_t cmd = Serial.read();

    // Function
    if(cmd == SEARCH_UPD)       getVersion();
    else if(cmd == UPDATE)      getUpdate();
    else if(cmd == VERSION)     Serial.write(VERSION_INT);
    else if(cmd == SCAN)        getScan();
    else if(cmd == ADD_WIFI)    wifiAddAP();
    else if(cmd == MULTI_WIFI)  wifiAddMultiAP();
    else if(cmd == TCP_CONNECT) connectServer();
    else if(cmd == NTP_TIME)    getTime();
    // System
    else if(cmd == INTERNET)    Serial.write(internet_connect() ? CMD_OK : CMD_ERROR);
    else if(cmd == STATION)     Serial.write(station_connect() ? CMD_OK : CMD_ERROR);
    else if(cmd == WIFI_STATUS) Serial.write(wifi_station_get_connect_status());
  }

  delay(10);
}


void getVersion() {
  if (!test_connection()) return; // Test the connections

  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  // If you have finger print
  //client->setFingerprint(fingerprint);
  client->setInsecure();

  HTTPClient https;
  const String _board[2] = {"esp8266.txt", "rpi_pico.txt"};
  uint16_t _version = 0;
  uint8_t _select = 1;

  do {
    https.begin(*client, F("raw.githubusercontent.com"), 443, F("/alexElArte/Phone/main/update/") + _board[_select], true);
    int _httpCode = https.GET();
    if (_httpCode == HTTP_CODE_OK) {
      String payload = https.getString();
      uint8_t _data[2];
      _data[0] = payload.substring(0, payload.indexOf('.')).toInt();
      _data[1] = payload.substring(payload.indexOf('.') + 1, payload.length()).toInt();
      _version |= (_data[0] << 4 + _data[1]) << (_select * 8);
    }
    https.end();
  } while (_select--);

  Serial.write(_version>>8);
  Serial.write(_version);
}

void getScan() {
  String _ssid;
  int32_t _rssi;
  uint8_t _encryptionType;
  uint8_t* _bssid;
  int32_t _channel;
  bool _hidden;
  int _scanResult;

  // Start wifi scan
  _scanResult = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);

  if (_scanResult == 0) {
    Serial.write(0);
  } else if (_scanResult > 0) {
    Serial.write(_scanResult);

    // Print unsorted scan results
    for (int8_t _i = 0; _i < _scanResult; _i++) {
      WiFi.getNetworkInfo(_i, _ssid, _encryptionType, _rssi, _bssid, _channel, _hidden);
      Serial.print(_ssid);
      Serial.write('/');
      writeUint32_t(_rssi);
      Serial.write(_encryptionType);
      Serial.write(_bssid, 4);
      writeUint32_t(_channel);
      Serial.write(_hidden);
      Serial.write('#');
      delay(0);
    }
  } else {
    Serial.write(CMD_ERROR); // Error
  }
}

void wifiAddAP() {
  char* _ssid = "";
  char* _psw = "";
  Serial.readBytesUntil('/', _ssid, 33);
  Serial.readBytesUntil('/', _psw, 33);
  bool check = WiFiMulti.addAP(_ssid, _psw);

  Serial.write(check ? CMD_OK : CMD_ERROR);
}

void wifiAddMultiAP() {
  uint8_t _count = Serial.read();
  if (_count == 0) return;
  for (uint8_t _i = 0; _i < _count; _i++) {
    wifiAddAP();
  }
}

void connectServer() {
  if (!test_connection()) return; // Test the connections

  char* _host = "";
  uint16_t _port = 0;
  Serial.readBytesUntil('/', _host, 30);
  _port = (Serial.read() << 8) | Serial.read();

  WiFiClient client;
  if (!client.connect(_host, _port)) {
    Serial.write(CMD_ERROR);
    return;
  }
  Serial.write(CMD_OK);
  while (client.connected()) {
    if (Serial.available()) {
      uint8_t _byte = Serial.read();
      if (_byte == 0xff) break;
      client.write(_byte);
    }
    if (client.available()) Serial.write(client.read());
    delay(1);
  }
  client.stop();
  Serial.write(CMD_ERROR);
}

void getUpdate() {
  if (!test_connection()) return; // Test the connections

  String _select_board = "";
  uint8_t _board = Serial.read();
  if (_board == RPI_PIC) _select_board = "rpi_pico/";
  else if (_board == ESP8266) _select_board = "esp8266/";
  else {
    Serial.write(CMD_ERROR);
    return;
  }
  Serial.write(CMD_OK);

  uint8_t _version = Serial.read();
  String _select_version = _select_board + String(_version >> 4) + "." + String(_version & 0xf) + ".bin";


  std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  // If you have finger print
  //client->setFingerprint(fingerprint);
  client->setInsecure();

  HTTPClient https;
  https.begin(*client, F("raw.githubusercontent.com"), 443, F("/alexElArte/Phone/main/update/") + _select_version, true);
  int _httpCode = https.GET();
  if (_httpCode == HTTP_CODE_OK && https.getSize() > 0) {
    Serial.write(CMD_OK);
    uint32_t _len = https.getSize();
    writeUint32_t(_len);

    uint8_t buff[256] = { 0 };
    while (https.connected() && (_len > 0 || _len == -1)) {
      uint16_t c = client->readBytes(buff, std::min((size_t)_len, sizeof(buff)));
      Serial.write(buff, c);
      if (_len > 0) _len -= c;
      delay(0);
    }
  } else {
    Serial.write(CMD_ERROR);
  }

  https.end();
}

void getTime() {
  if (!test_connection()) return; // Test the connections

  // For more information see NTPClient.ino
  const int NTP_PACKET_SIZE = 48;
  uint8_t packetBuffer[NTP_PACKET_SIZE];

  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  WiFiUDP udp;
  udp.begin(2390);
  udp.beginPacket("time.nist.gov", 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();

  uint32_t _now = millis();
  while (!udp.parsePacket() && (millis() - _now < NTP_TIMEOUT)) delay(10);
  uint16_t _cb = udp.parsePacket();
  if (_cb) udp.read(packetBuffer, NTP_PACKET_SIZE);

  for (uint8_t _i = 40; _i < 44; _i) {
    Serial.write(packetBuffer[_i]);
  }
  udp.stop();
}
