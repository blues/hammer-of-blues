#include <Arduino.h>
#include <ButtonDebounce.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_LIS3DH.h>
#include <IRremote.hpp>
#include <Adafruit_NeoPixel.h>
#include "pitches.h"

#define SIDE_BUTTON_PIN A2
#define BACK_BUTTON_PIN A3
#define BUZZER_PIN A0
#define IR_SEND_PIN D13
#define IR_RECEIVE_PIN D9
#define NEOPIXEL_PIN A1

#define NUM_PIXELS 2

#define SCREEN_ADDRESS 0x3C
#define BME280_ADDRESS 0x76
#define LIS3DH_ADDRESS 0x18

// Buzzer implementation

// Mario main theme melody
int melody[] = {NOTE_E7, NOTE_E7, 0, NOTE_E7, 0, NOTE_C7, NOTE_E7, 0, NOTE_G7, 0, 0, 0, NOTE_G6};

// Mario main theme tempo
int tempo[] = {12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12};

ButtonDebounce sideButton(SIDE_BUTTON_PIN, 200);
ButtonDebounce backButton(BACK_BUTTON_PIN, 200);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bmeSensor;
Adafruit_LIS3DH lisSensor = Adafruit_LIS3DH();
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void sideButtonChanged(const int state)
{
  if (state == 1)
  {
    Serial.println("Button Verified: " + String(state));
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void backButtonChanged(const int state)
{
  if (state == 0)
  {
    Serial.println("Button Verified: " + String(state));
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void theaterChase(uint32_t color, int wait)
{
  for (int a = 0; a < 10; a++)
  { // Repeat 10 times...
    for (int b = 0; b < 3; b++)
    {                 //  'b' counts from 0 to 2...
      pixels.clear(); //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for (int c = b; c < pixels.numPixels(); c += 3)
      {
        pixels.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      pixels.show(); // Update strip with new contents
      delay(wait);   // Pause for a moment
    }
  }
}

void playMarioTheme()
{
  int size = sizeof(melody) / sizeof(int);
  for (int thisNote = 0; thisNote < size; thisNote++)
  {
    // to calculate the note duration, take one second divided by the note type
    // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc
    int noteDuration = 1000 / tempo[thisNote];

    // start the tone
    tone(BUZZER_PIN, melody[thisNote], noteDuration);
    delay(noteDuration);

    // stop the tone
    noTone(BUZZER_PIN, true);

    // to distinguish the notes, set a minimum time between them
    delay(noteDuration);
  }

  noTone(BUZZER_PIN, true);
}

void setup()
{
  Serial.begin(115200);
  delay(2500);
  Serial.println("============================");
  Serial.println("Hammer of Blues Verification");
  Serial.println("============================");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  Serial.println("Screen Connected");

  display.setRotation(2);
  display.clearDisplay();

  playMarioTheme();

  IrSender.begin(IR_SEND_PIN);
  IrReceiver.begin(IR_RECEIVE_PIN);

  pixels.begin();
  pixels.show();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Hammer of Blues!"));
  display.display();

  unsigned bmeStatus = bmeSensor.begin(BME280_ADDRESS, &Wire);
  if (!bmeStatus)
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring, I2C address");
  }
  else
  {
    Serial.println("BME280 Connected");
  }

  display.setCursor(0, 10);
  display.print("Temp: ");
  display.print(bmeSensor.readTemperature());
  display.println(" C");
  display.display();

  if (!lisSensor.begin(LIS3DH_ADDRESS))
  {
    Serial.println("Could not find a valid LIS3DH Sensor, check wiring, I2C address");
  }
  Serial.println("LIS3DH Connected");

  lisSensor.setRange(LIS3DH_RANGE_8_G);
  lisSensor.read();
  display.setCursor(0, 20);
  display.print("X: ");
  display.print(lisSensor.x);
  display.print(" Y: ");
  display.print(lisSensor.y);
  display.print(" Z: ");
  display.println(lisSensor.z);
  display.display();

  sideButton.setCallback(sideButtonChanged);
  backButton.setCallback(backButtonChanged);

  theaterChase(pixels.Color(0, 0, 127), 50); // Blue

  digitalWrite(LED_BUILTIN, LOW);
}

uint16_t sAddress = 0x0102;
uint8_t sCommand = 0x34;
uint8_t sRepeats = 0;

void loop()
{

  sideButton.update();
  backButton.update();

  /*

  Serial.println();
  Serial.print(F("IR Send now: address=0x"));
  Serial.print(sAddress, HEX);
  Serial.print(F(" command=0x"));
  Serial.print(sCommand, HEX);
  Serial.print(F(" repeats="));
  Serial.print(sRepeats);
  Serial.println();

  IrSender.sendNEC(sAddress, sCommand, sRepeats);

  if (IrReceiver.decode()) {
    IrReceiver.printIRResultShort(&Serial);
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
      IrReceiver.printIRResultRawFormatted(&Serial, true);
    }
    Serial.println();

    IrReceiver.resume(); // Enable receiving of the next value

    if (IrReceiver.decodedIRData.command == sCommand) {
      Serial.println("IR Communication verified");
    } else if (IrReceiver.decodedIRData.command == 0x11) {
      Serial.println("IR Signal received, but it's not from our emitter...");
    }
  }

  pixels.clear();

  for(int i=0; i<NUM_PIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 150));
    pixels.show();
  }

  delay(5000);
  */
}
