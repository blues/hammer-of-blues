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

#define BUTTON_PIN     A2
#define BUZZER_PIN     A0
#define IR_SEND_PIN    D13
#define IR_RECEIVE_PIN D9
#define NEOPIXEL_PIN   A1

#define NUM_PIXELS 2

#define SCREEN_ADDRESS 0x3C
#define BME280_ADDRESS 0x76
#define LIS3DH_ADDRESS 0x18

// Buzzer implementation note
// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_C5, NOTE_C6, NOTE_C7, NOTE_C8, NOTE_C7, NOTE_C6, NOTE_C5, NOTE_C4
};

ButtonDebounce button(BUTTON_PIN, 200); // PIN D0 with 2500ms debounce time

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bmeSensor;
Adafruit_LIS3DH lisSensor = Adafruit_LIS3DH();
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(2500);
  Serial.println("Hammer of Blues Verification");
  Serial.println("============================");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  Serial.println("Screen Connected");

  display.clearDisplay();

  noTone(BUZZER_PIN);
  tone(BUZZER_PIN, 0, 500);
  delay(250);
  
  noTone(BUZZER_PIN);
  
  for (int thisNote = 0; thisNote < 9; thisNote++) {
    tone(BUZZER_PIN, melody[thisNote], 500);

    delay(250);
    noTone(BUZZER_PIN);
  }
  delay(250);
  
  noTone(BUZZER_PIN);

  IrSender.begin(IR_SEND_PIN, ENABLE_LED_FEEDBACK);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  pixels.begin();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("Hammer of Blues!"));
  display.display();

  unsigned bmeStatus = bmeSensor.begin(BME280_ADDRESS, &Wire);  
  if (!bmeStatus) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, I2C address");
  } else {
    Serial.println("BME280 Connected");
  }

  display.setCursor(0,10);
  display.print("Temp: ");
  display.print(bmeSensor.readTemperature());
  display.println(" C");
  display.display();

  if (!lisSensor.begin(LIS3DH_ADDRESS)) {
    Serial.println("Could not find a valid LIS3DH Sensor, check wiring, I2C address");
  }
  Serial.println("LIS3DH Connected");

  lisSensor.setRange(LIS3DH_RANGE_8_G);
  lisSensor.read(); 
  display.setCursor(0,20);
  display.print("X: ");
  display.print(lisSensor.x);
  display.print(" Y: ");
  display.print(lisSensor.y);
  display.print(" Z: ");
  display.println(lisSensor.z);
  display.display();
  
  button.setCallback(buttonChanged);
  
  digitalWrite(LED_BUILTIN, LOW);
}

uint16_t sAddress = 0x0102;
uint8_t sCommand = 0x34;
uint8_t sRepeats = 0;

void loop() {
  button.update();

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
}

void buttonChanged(const int state) {
  if (state == 1) {
    Serial.println("Button Verified: " + String(state));
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
