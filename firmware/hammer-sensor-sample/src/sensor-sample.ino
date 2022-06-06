#include <Arduino.h>
#include <Notecard.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define BME280_ADDRESS 0x76

// pressure at the sea level in hectopascal (is equivalent to milibar)
#define SEALEVELPRESSURE_HPA (1013.25)

#define PRODUCT_UID "<your notehub productuid>"
Notecard notecard;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bmeSensor;

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 1000 * 60;

void setup()
{
  Serial.begin(115200);
  delay(20000); // time to get serial monitor open!
  Serial.println("===============");
  Serial.println("Hammer of Blues Sensor Tutorial!");
  Serial.println("===============");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Wire.begin();
  notecard.begin();

  J *req = notecard.newRequest("hub.set");
  JAddStringToObject(req, "product", PRODUCT_UID);
  JAddStringToObject(req, "sn", "blues-hammer");
  JAddStringToObject(req, "mode", "continuous");
  notecard.sendRequest(req);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  Serial.println("Screen Connected");

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Hammer of Blues Sensor Tutorial!"));
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

  digitalWrite(LED_BUILTIN, LOW);
  startMillis = millis();
}

void loop()
{
  currentMillis = millis();
  if (currentMillis - startMillis >= period)
  {

    Serial.println("Ready to read from BME 280!");

    display.clearDisplay();

    long temp = bmeSensor.readTemperature();
    long humidity = bmeSensor.readHumidity();
    long pressure = bmeSensor.readPressure();
    // estimates altitude in meters based on the pressure at the sea level
    long altitude = bmeSensor.readAltitude(SEALEVELPRESSURE_HPA);

    // write data out to serial monitor
    Serial.print("Temp: ");
    Serial.print(temp);
    Serial.println(" C");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");
    Serial.print("Altitude: ");
    Serial.print(altitude);
    Serial.println(" m");

    // Display sensor data to screen
    display.setCursor(0, 10);
    display.print("Temp: ");
    display.print(temp);
    display.println(" C");

    display.print("Humidity: ");
    display.print(humidity);
    display.println("%");
    display.display();

    display.print("Pressure: ");
    display.print(pressure);
    display.println(" hPa");
    display.display();

    display.print("Altitude: ");
    display.print(altitude);
    display.println(" m");
    display.display();

    J *req = notecard.newRequest("note.add");
    if (req != NULL)
    {
      JAddBoolToObject(req, "sync", true);
      J *body = JCreateObject();
      if (body != NULL)
      {
        JAddNumberToObject(body, "temp", temp);
        JAddNumberToObject(body, "humidity", humidity);
        JAddNumberToObject(body, "pressure", pressure);
        JAddNumberToObject(body, "altitude", altitude);
        JAddItemToObject(req, "body", body);
      }
      notecard.sendRequest(req);
    }

    startMillis = currentMillis;
  }
}