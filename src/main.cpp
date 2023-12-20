#include <Arduino.h>
#include <WiFi.h>
#include "NTPClient.h"
#include <WiFiUdp.h>
#include "esp_timer.h"
#include "creds.h"

#define M_SDI 26
#define M_CLK 27
#define M_RCLK 14
#define M_OE 12
int counter = 0;

// const char* ntpServer = "pool.ntp.org";
const char* ntpServer = "192.168.7.1";
const long  gmtOffset_sec = -8*60*60;
const int   daylightOffset_sec = 0;
const char* cstOffset = "-8";
struct tm timeinfo;
unsigned long lastEpoch = 0;
int deltaTimerToEpoch = 0;
int time_now = 0;
int prev_time = 0;

uint8_t display[6];
uint8_t numbers[] = {
  0b11111100,
  0b01100000,
  0b11011010,
  0b11110010,
  0b01100110,
  0b10110110,
  0b10111110,
  0b11100000,
  0b11111110,
  0b11110110
};

int pow(int x, unsigned int y) {
  if (y == 0)
    return 1;
  else if ((y % 2) == 0)
    return pow(x, y / 2) * pow(x, y / 2);
  else
    return x * pow(x, y / 2) * pow(x, y / 2);
}

void sdi(bool state) {
  if (state) {
    digitalWrite(M_SDI, HIGH);
  } else {
    digitalWrite(M_SDI, LOW);
  }
}

void clk() {
  digitalWrite(M_CLK, HIGH);
  digitalWrite(M_CLK, LOW);
}

void rclk() {
  digitalWrite(M_RCLK, HIGH);
  digitalWrite(M_RCLK, LOW);
}

void clockDisplay() {
  for (int i = 0; i < 6; i++) {
    for (int x = 0; x < 8; x++) {
      sdi((display[6-(i+1)] >> x) & 0x01);
      clk();
    }
  }
}

void updateDisplay(uint8_t pattern, int position) {
  display[position] = pattern;
}

int getHour(unsigned long epoch) {
  return ((epoch % 86400L) / 3600);
}

int getMinute(unsigned long epoch) {
  return ((epoch % 3600) / 60);
}

int getSecond(unsigned long epoch) {
  return (epoch % 60);
}

int getTimerTime() {
  return esp_timer_get_time()/(int)pow(10, 6);
}

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, ntpServer, gmtOffset_sec);

// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup() {

  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // TODO loading animation
  // for (int i = 0; i < 50; i++) {
    
  //   delay(500);
  // }
  Serial.println("");
  Serial.println("WiFi connected.");
  
  // Init and get the time
  timeClient.begin();
  timeClient.update();
  lastEpoch = timeClient.getEpochTime();
  deltaTimerToEpoch = getTimerTime();
  prev_time = lastEpoch;
  timeClient.end();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);


  pinMode(M_SDI, OUTPUT);
  pinMode(M_CLK, OUTPUT);
  pinMode(M_RCLK, OUTPUT);
  pinMode(M_OE, OUTPUT);
  digitalWrite(M_OE, LOW);
  digitalWrite(M_CLK, LOW);
  digitalWrite(M_RCLK, LOW);
  digitalWrite(M_SDI, LOW);
  for (int i = 0; i < 6; i++) {
    display[i] = 0;
  }
  for (int i = 0; i < 48; i++) {
    clk();
  }
  rclk();
}

void loop() {
  int timer_now = getTimerTime();
  time_now = lastEpoch+(timer_now-deltaTimerToEpoch);

  if (time_now != prev_time) {
    prev_time = time_now;
    updateDisplay(numbers[getHour(time_now)/10], 0);
    updateDisplay(numbers[getHour(time_now)%10], 1);
    updateDisplay(numbers[getMinute(time_now)/10], 2);
    updateDisplay(numbers[getMinute(time_now)%10], 3);
    updateDisplay(numbers[getSecond(time_now)/10], 4);
    updateDisplay(numbers[getSecond(time_now)%10], 5);
    clockDisplay();
    rclk();
  }
}