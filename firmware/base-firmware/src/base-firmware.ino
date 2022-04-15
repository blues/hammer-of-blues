#include <Notecard.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_LIS3DH.h>

#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define BME280_I2C_ADDRESS 0x76
#define LIS3DH_I2C_ADDRESS 0x18

#define PRODUCT_UID "com.blues.bsatrom:hammer_of_blues"
Notecard notecard;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bmeSensor;
Adafruit_LIS3DH lisSensor = Adafruit_LIS3DH();

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

  digitalWrite(LED_BUILTIN, LOW);
  startMillis = millis();
}

void loop() {
  currentMillis = millis();
  if (currentMillis - startMillis >= period) {
    display.clearDisplay();

    long temp = bmeSensor.readTemperature();
    long humidity = bmeSensor.readHumidity();

    //Clear the screen

    //Get current temp and humidity and send to Notecard
    //Display temp and HU to screen
    display.setCursor(0,10);
    display.print("Temp: ");
    display.print(temp);
    display.println(" C");

    display.print("Humidity: ");
    display.print(humidity);
    display.println("%");
    display.display();

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
