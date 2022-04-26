#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN    A1
#define NUM_PIXELS 2

Adafruit_LIS3DH lis = Adafruit_LIS3DH();
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

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

void setup()
{
  delay(2500);
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  ei_printf("Edge Impulse Inferencing Demo\n");
  Wire.begin();

  if (!lis.begin(0x18))
  { // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start");
    while (1)
      yield();
  }
  Serial.println("LIS3DH found!");

  pixels.begin();
  pixels.show();

  lis.setDataRate(LIS3DH_DATARATE_400_HZ);

  if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3)
  {
    ei_printf("ERR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (the 3 sensor axes)\n");
    return;
  }

  theaterChase(pixels.Color(255,0,0), 50); // Red
}

void ei_printf(const char *format, ...)
{
  static char print_buf[1024] = {0};

  va_list args;
  va_start(args, format);
  int r = vsnprintf(print_buf, sizeof(print_buf), format, args);
  va_end(args);

  if (r > 0)
  {
    Serial.write(print_buf);
  }
}

void loop()
{
  ei_printf("\nStarting inferencing in 2 seconds...\n");
  delay(2000);

  lis.read();
  Serial.print(lis.x);
  Serial.print("\t");
  Serial.print(lis.y);
  Serial.print("\t");
  Serial.print(lis.z);
  Serial.print("\n");

  ei_printf("Sampling...\n");

  // Allocate a buffer here for the values we'll read from the IMU
  static int16_t rawReadingBuf[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {0};
  static float readingBuf[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

  for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3)
  {
    // Determine the next tick (and then sleep later)
    uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

    lis.read();

    rawReadingBuf[ix] = lis.x;
    rawReadingBuf[ix + 1] = lis.y;
    rawReadingBuf[ix + 2] = lis.z;

    delayMicroseconds(next_tick - micros());
  }

  // Turn the raw buffer in a signal which we can the classify
  numpy::int16_to_float(rawReadingBuf, readingBuf, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
  signal_t signal;
  int err = numpy::signal_from_buffer(readingBuf, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
  if (err != 0)
  {
    ei_printf("Failed to create signal from buffer (%d)\n", err);
    return;
  }

  // Run the classifier
  ei_impulse_result_t result = {0};

  err = run_classifier(&signal, &result, debug_nn);
  if (err != EI_IMPULSE_OK)
  {
    ei_printf("ERR: Failed to run classifier (%d)\n", err);
    return;
  }

  ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)\n",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);
  uint8_t predictionLabel = 0;
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
  {
    Serial.print("    ");
    Serial.print(result.classification[ix].label);
    Serial.print(": ");
    Serial.println(result.classification[ix].value);

    if (result.classification[ix].value > result.classification[predictionLabel].value)
      predictionLabel = ix;
  }

  // print the predictions
  String label = result.classification[predictionLabel].label;

  Serial.print("\nPrediction: ");
  Serial.println(label);

  if (label == "idle") {
    theaterChase(pixels.Color(0,255,0), 50); // Green
  } else if (label == "chop") {
    theaterChase(pixels.Color(255,0,0), 50); // Red
  } else if (label == "circle") {
    theaterChase(pixels.Color(0,0,255), 50); // Blue
  }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
  Serial.print("    ");
  Serial.print("anomaly score: ");
  Serial.print(result.anomaly);
#endif
}