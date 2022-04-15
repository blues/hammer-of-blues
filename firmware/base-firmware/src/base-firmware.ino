#include <Notecard.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_NeoPixel.h>

#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define NEOPIXEL_PIN  A1

#define NUM_PIXELS 2

#define BME280_I2C_ADDRESS 0x76
#define LIS3DH_I2C_ADDRESS 0x18

#define PRODUCT_UID "com.blues.bsatrom:hammer_of_blues"
Notecard notecard;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bmeSensor;
Adafruit_LIS3DH lisSensor = Adafruit_LIS3DH();
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 360000;

void setup() {
  Serial.begin(115200);
  delay(2500);
  Serial.println("Hammer of Blues");
  Serial.println("===============");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Wire.begin();
  notecard.begin();

  J *req = notecard.newRequest("hub.set");
  JAddStringToObject(req, "product", PRODUCT_UID);
  JAddStringToObject(req, "sn", "jank-hammer");
  JAddStringToObject(req, "mode", "continuous");
  notecard.sendRequest(req);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  Serial.println("Screen Connected");

  display.setRotation(2);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("Hammer of Blues!"));
  display.display();

  unsigned bmeStatus = bmeSensor.begin(BME280_I2C_ADDRESS, &Wire);
  if (!bmeStatus) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, I2C address");
  } else {
    Serial.println("BME280 Connected");
  }

  if (!lisSensor.begin(LIS3DH_I2C_ADDRESS)) {
    Serial.println("Could not find a valid LIS3DH Sensor, check wiring, I2C address");
  }
  Serial.println("LIS3DH Connected");

  pixels.begin();
  pixels.show();

  theaterChase(pixels.Color(  0,   0, 255), 50); // Blue

  digitalWrite(LED_BUILTIN, LOW);

  digitalWrite(LED_BUILTIN, LOW);
  startMillis = millis();
}

void loop() {
  currentMillis = millis();

  //Clear the screen
  display.clearDisplay();

  // Read from Accel and Write to Screen
  lisSensor.setRange(LIS3DH_RANGE_8_G);
  lisSensor.read();
  display.setCursor(0,0);
  display.print("X: ");
  display.println(lisSensor.x);
  display.print("Y: ");
  display.println(lisSensor.y);
  display.print("Z: ");
  display.println(lisSensor.z);
  display.display();

  if (currentMillis - startMillis >= period) {
    //Get current temp and humidity and send to Notecard
    long temp = bmeSensor.readTemperature();
    long humidity = bmeSensor.readHumidity();

    J *req = notecard.newRequest("note.add");
    if (req != NULL) {
        JAddBoolToObject(req, "sync", true);
        J *body = JCreateObject();
        if (body != NULL) {
            JAddNumberToObject(body, "temp", temp);
            JAddNumberToObject(body, "humidity", humidity);
            JAddItemToObject(req, "body", body);
        }
        notecard.sendRequest(req);
    }

    startMillis = currentMillis;
  }
}

void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      pixels.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<pixels.numPixels(); c += 3) {
        pixels.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      pixels.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}
