#include <TimeLib.h> 
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include "SDWebServer.h"
#include "ting.h"

#define PIN            9    // Node-MCU Input Pin
#define NUMPIXELS      26    // Number of Pixels on the Ring
#define BRIGHTNESS     30    // Brightness between 0 - 255

const char ssid[] = "";       //  your network SSID (name)
const char pass[] = "";       // your network password

int loopCount = 0;
int animationCount = 0;
bool doAnimate = false;
bool doMessage = false;
bool doClear = false;


Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
                    // green, red, blue
uint32_t red = pixels.Color(0, 255, 0);
uint32_t green = pixels.Color(255, 0, 0);
uint32_t blue = pixels.Color(0, 0, 255);
uint32_t yellow = pixels.Color(200, 255, 0);
uint32_t orange = pixels.Color(100, 255, 0);
uint32_t purple = pixels.Color(0, 255, 240);
uint32_t teal = pixels.Color(255, 0, 255);

const int maxColors = 7;
const uint32_t colors[maxColors] = {red, green, blue, yellow, orange, purple, teal};

// map characters A-Z to an LED
const int characterMap[26] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};

const int bufSize = 500;
char messageBuffer[bufSize];  // ring buffer for messages
int bufReadIndex = 0;     // read pointer in ring buffer
int bufWriteIndex = 0;    // write pointer in ring buffer
int bufLength = 0;        // length of current message

uint32_t GetRandomColor(void) {
  int index = random(0, maxColors);
  Serial.print("Color:");
  Serial.println(index);
  return colors[index];
}

bool WriteToBuffer(char c) {
  if(!IsBufferFull()) {
    messageBuffer[bufWriteIndex] = c;
    bufLength ++;
    bufWriteIndex ++;
    if(bufWriteIndex >= bufSize) {
      bufWriteIndex = 0;
    }
    return true;
  }
  return false;
}

char ReadFromBuffer(void) {
  char ret = 0;
  if(!IsBufferEmpty()) {
    ret = messageBuffer[bufReadIndex];
    bufLength --;
    bufReadIndex ++;
    if(bufReadIndex >= bufSize) {
      bufReadIndex = 0;
    }
  }
  return ret;
}

bool IsBufferEmpty(void) {
  return bufLength == 0;
}

bool IsBufferFull(void) {
  return bufLength == bufSize;
}

// only allow characters A-Z
// convert lowercase to uppercase
// also allows space, which is ignored by leds
char GetValidCharacter(char c) {
  if(c >= 'a' && c <= 'z') return c - 0x20;
  if(c >= 'A' && c <= 'Z') return c;
  if(c == ' ') return c;
  return 0;
}

// convert character A-Z to led index
int GetLedIndex(char c) {
  if(c < 'A' || c > 'Z') {
    return -1;
  }
  return characterMap[c - 'A'];
}

void ClearLeds(void) {
  int i = 0;
  for(i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, 0, 0, 0);
  }
}

void AnimateLeds(void) {
  for(int i = 0; i < NUMPIXELS; i++) {
    uint32_t color = (random(100, 255) << 16) + (random(100, 255) << 8) + random(100, 255);
    pixels.setPixelColor(i, color);
  }
}

void setup() {
  Serial.begin(9600);
  delay(250);
  Serial.println("Stranger Things Lights!");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  pixels.begin();
  
  
  pixels.setBrightness(BRIGHTNESS);
  delay(500);

  ClearLeds();
  pixels.show();
  SetupWebServer();
}

void loop() {  
  ClearLeds();
  WebServerLoop();
  if(doAnimate) {
    AnimateLeds();
    if(++animationCount >= 30) {
      animationCount = 0;
      doAnimate = false;
      ClearLeds();
    }
    pixels.show();
  }
  else if(++loopCount >= 8) { 
    loopCount = 0;
    // leds off between letters
    if(doClear) {
      doClear = false;
      pixels.show();
    } else if(!IsBufferEmpty()) {
      doMessage = true;
      char c = ReadFromBuffer();
      int index = GetLedIndex(c);
      if(index >= 0) {
        pixels.setPixelColor(index, GetRandomColor());
        pixels.show();
      } 
      doClear = true;
    } else if(doMessage) {
      doMessage = false;
      doAnimate = true;
    }
  }
  
  delay(100);
}

// Get message from user and add to buffer
void StrangerThingsCallback(String text) {
  Serial.println("Received: " + text);
  int i = 0;
  for(i = 0; i < text.length(); i++) {
    char c = text[i];
    c = GetValidCharacter(c);
    if(c > 0) {
      WriteToBuffer(c);
    }
  }
}

