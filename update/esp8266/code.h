// Function
#define SEARCH_UPD  0x01 // Search for update
#define UPDATE      0x02 // Get update file
#define VERSION     0x03 // Get esp8266 version
#define SCAN        0x04 // Get wifi scan
#define ADD_WIFI    0x05 // Add wifi connection
#define MULTI_WIFI  0x06 // Add several wifi connection
#define TCP_CONNECT 0x07 // Connect to a tcp server
#define NTP_TIME    0x08 // Get current time

// System
#define INTERNET    0x09 // Is there an internet connection
#define STATION     0x0A // Is there a station
#define WIFI_STATUS 0x0B

// Status
#define CMD_ERROR   0xFF
#define CMD_FINISH  0x01
#define CMD_OK      0x02
#define LOADING     0x03
#define NO_INTERNET 0x04
#define NO_WIFI     0x05

// Board
#define RPI_PIC 0x01 // Select the raspberry pi pico
#define ESP8266 0x02 // Select the esp8266

#define NTP_TIMEOUT 2500

void writeUint32_t(uint32_t _t) {
  Serial.write(_t >> 24);
  Serial.write(_t >> 16);
  Serial.write(_t >> 8);
  Serial.write(_t);
}
bool station_connect() {
  if (WiFiMulti.run() == WL_CONNECTED) return true;
  return false;
}
bool internet_connect() {
  WiFiClient test_client;
  if (test_client.connect("www.google.com", 80) || test_client.connect("www.google.com", 443)) {
    test_client.stop();
    return true;
  }
  return false;
}
bool test_connection() {
  if (!station_connect()) {
    Serial.write(NO_WIFI);
    return false;
  }
  if (!internet_connect()) {
    Serial.write(NO_INTERNET);
    return false;
  }
  Serial.write(CMD_OK);
  return true;
}