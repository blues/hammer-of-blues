#include <Adafruit_LIS3DH.h>
#include <Adafruit_NeoPixel.h>
#include <Hammer_XL_inferencing.h> // replace this with reference to YOUR model

#define NEOPIXEL_PIN A1
#define NUM_PIXELS 2

static bool debug_nn = false; // Set this to true to see features generated from raw signal

Adafruit_LIS3DH lis = Adafruit_LIS3DH();
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void theaterChase(uint32_t color, int wait)
{
    for (int a = 0; a < 20; a++)
    {
        for (int b = 0; b < 3; b++)
        {                   //  'b' counts from 0 to 2...
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

void setup()
{
    Serial.begin(115200);
    lis.begin(0x18);
    pixels.begin();
    pixels.show();
    theaterChase(pixels.Color(255, 255, 255), 50); // white

    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3)
    {
        ei_printf("ERR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (the 3 sensor axes)\n");
        return;
    }
}

void loop()
{
    ei_printf("\nStarting inferencing in 2 seconds...\n");
    delay(2000);
    ei_printf("Sampling...\n");

    // Allocate a buffer here for the values we'll read from the IMU
    float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {0};

    for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3)
    {
        // Determine the next tick (and then sleep later)
        uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

        lis.read();
        buffer[ix] = lis.x;
        buffer[ix + 1] = lis.y;
        buffer[ix + 2] = lis.z;

        // Serial.print(lis.x);
        // Serial.print("\t");
        // Serial.print(lis.y);
        // Serial.print("\t");
        // Serial.print(lis.z);
        // Serial.println();

        delayMicroseconds(next_tick - micros());
    }

    // Turn the raw buffer in a signal which we can the classify
    signal_t signal;
    int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
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

    // engage neopixels!
    if (label == "idle")
    {
        theaterChase(pixels.Color(255, 0, 0), 50); // Green (GRB)
    }
    else if (label == "chop")
    {
        theaterChase(pixels.Color(0, 255, 0), 50); // Red (GRB)
    }
    else if (label == "wave")
    {
        theaterChase(pixels.Color(0, 0, 255), 50); // Blue (GRB)
    }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
    ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif
}