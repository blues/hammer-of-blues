// Basic demo for accelerometer readings from Adafruit LIS3DH

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

Adafruit_LIS3DH lis = Adafruit_LIS3DH();

void setup(void)
{
  Serial.begin(115200);
  lis.begin(0x18);
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
}