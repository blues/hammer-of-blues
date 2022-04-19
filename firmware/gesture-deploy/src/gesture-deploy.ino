#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

Adafruit_LIS3DH lis = Adafruit_LIS3DH();

static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

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

  lis.setDataRate(LIS3DH_DATARATE_400_HZ);

  if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3)
  {
    ei_printf("ERR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (the 3 sensor axes)\n");
    return;
  }
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
  Serial.print("\nPrediction: ");
  Serial.println(result.classification[predictionLabel].label);
#if EI_CLASSIFIER_HAS_ANOMALY == 1
  Serial.print("    ");
  Serial.print("anomaly score: ");
  Serial.print(result.anomaly);
#endif
}