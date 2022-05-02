// Basic demo for accelerometer readings from Adafruit LIS3DH

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_LIS3DH lis = Adafruit_LIS3DH();

#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup(void)
{
  Serial.begin(115200);

  if (!lis.begin(0x18))
  { // change this to 0x19 for alternative i2c address
    Serial.println("Couldn't start LIS3DH");
    while (1)
      yield();
  }
  Serial.println("LIS3DH found!");

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setRotation(2);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Screen Connected"));
  display.display();
  delay(2000);
}

void loop()
{
  lis.read(); // get x,y,z data at once
  Serial.print(lis.x);
  Serial.print("\t");
  Serial.print(lis.y);
  Serial.print("\t");
  Serial.print(lis.z);
  Serial.println();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("X: ");
  display.println(lis.x);
  display.print("Y: ");
  display.println(lis.y);
  display.print("Z: ");
  display.println(lis.z);
  display.display();
}