#include <Wire.h>
#include <VL53L0X.h>
#include <FastLED.h>

#define NUM_SENSORS 10
#define NUM_LEDS 18
#define DATA_PIN 2
#define MAX_BRIGHTNESS 255

// XSHUT pins for each sensor
const uint8_t xshutPins[NUM_SENSORS] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
const uint8_t sensorAddresses[NUM_SENSORS] = {
  0x30, 0x31, 0x32, 0x33, 0x34,
  0x35, 0x36, 0x37, 0x38, 0x39
};

int distances[NUM_SENSORS] = {1000};
uint8_t currentBrightness[NUM_LEDS] = {0};  // Store current brightness for fade-out

unsigned long prevTime = 0;

VL53L0X sensors[NUM_SENSORS];
CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);
  Wire.begin();

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  // Set all XSHUT pins LOW to disable all sensors
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(xshutPins[i], OUTPUT);
    digitalWrite(xshutPins[i], LOW);
  }
  delay(10);

  // Sequentially enable each sensor and assign it a unique I2C address
  for (int i = 0; i < NUM_SENSORS; i++) {
    digitalWrite(xshutPins[i], HIGH);
    delay(10);

    sensors[i].init();
    sensors[i].setAddress(sensorAddresses[i]);
    sensors[i].startContinuous();
  }

  Serial.println("All sensors initialized.");

  prevTime = millis();
}

void loop() {
  for (int i = 0; i < NUM_SENSORS; i++) {
    distances[i] = sensors[i].readRangeContinuousMillimeters();
  }
  delay(10);

  // not in conflict: 0, 2, 15, 17 (corners)
  updateLED(0, distances[0]);
  updateLED(2, distances[1]);
  updateLED(15, distances[9]);
  updateLED(17, distances[8]);

  // in conflict: 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16
  updateLED(1, (distances[0] < distances[1]) ? distances[0] : distances[1]);
  updateLED(3, (distances[1] < distances[3]) ? distances[1] : distances[3]);
  updateLED(5, (distances[0] < distances[2]) ? distances[0] : distances[2]);
  updateLED(6, (distances[2] < distances[4]) ? distances[2] : distances[4]);
  updateLED(8, (distances[3] < distances[5]) ? distances[3] : distances[5]);
  updateLED(9, (distances[5] < distances[7]) ? distances[5] : distances[7]);
  updateLED(11, (distances[4] < distances[6]) ? distances[4] : distances[6]);
  updateLED(12, (distances[6] < distances[8]) ? distances[6] : distances[8]);
  updateLED(14, (distances[7] < distances[9]) ? distances[7] : distances[9]);
  updateLED(16, (distances[8] < distances[9]) ? distances[8] : distances[9]);

  updateLED(4, min(min(distances[0], distances[1]), min(distances[2], distances[3])));
  updateLED(7, min(min(distances[2], distances[3]), min(distances[4], distances[5])));
  updateLED(10, min(min(distances[4], distances[5]), min(distances[6], distances[7])));
  updateLED(13, min(min(distances[6], distances[7]), min(distances[8], distances[9])));

  FastLED.show();
  delay(20);
}

void updateLED(int index, int distance) {
  if (distance > 400 || distance < 0) distance = 400;

  int targetBrightness = map(distance, 400, 0, 0, 255);
  targetBrightness = constrain(targetBrightness, 0, 255);

  // Smoothly fade to the target brightness
  if (currentBrightness[index] < targetBrightness) { // Fade in
    currentBrightness[index] = min(targetBrightness, currentBrightness[index] + 20); 
  } else if (currentBrightness[index] > targetBrightness) { // Fade out
    currentBrightness[index] = max(targetBrightness, currentBrightness[index] - 20); 
  }

  int hue = 20; 
  leds[index] = CHSV(hue, 255, currentBrightness[index]);
}