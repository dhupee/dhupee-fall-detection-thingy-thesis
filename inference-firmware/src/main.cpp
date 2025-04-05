#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

// * the ML model
#include <dh-thesis_inferencing.h>

// * Settings definition, uncomment the ones you want to use
// #define DRY_RUN // if this enabled it wont try to connect with SD Card module
// #define DEBUG_RUN // if this enable it will print the results

// * Assign IMU
Adafruit_MPU6050 mpu;

// * Pin Assign
const int BUTTON = 12; // Button for toggle
const int LED1 = 2;    // LED for send toggle notification
const int LED2 = 4;    // LED for whether the ESP is ready

const float G2MS2 = 9.80665f; // constant for converting g to m/s

// * VARIABLES
const int freq =
    5; // how delay on each loop, smaller the faster, in milliseconds
// const int freq = 1000; // use this for testing

// volatile bool sendToggle = true; // toggle for send the data to SD card
// int fileCounter = 1;

// int counter = 0;
const int sensorValues = 6; // AccX, AccY, AccZ, GyroX, GyroY, GyroZ
// const int bufferLength = 6;    // buffer for features fed to the model
const int featuresLength = 29; // length of the feature needed for the model

int startIndex = 0; // Points to the oldest element
int endIndex = 0;   // Points to the next insertion point

bool featuresReady = false; // Determines if the feature buffer is full

const float confidenceThreshold = 0.5; // confidence threshold of the model

// const float samplingInterval = 300;
// float lastTime;
// int sampleCounter = 1;

// * feature array, for being fed to the model
const int featuresSize = (featuresLength * sensorValues);
float features[featuresSize];

// // * sensor data array
// float sensorData[(featuresLength + bufferLength) * sensorValues];
// const int sensorDataSize = sizeof(sensorData) / sizeof(sensorData[0]);

/**
 * Function to blink specific LED pin on and off.
 *
 * @param delayTime the time between each blink
 * @param numberOfTimes the number of blinks
 * @param pin the pin number
 */
void blinkLoop(int delayTime, int numberOfTimes, int pin) {
  for (int i = 0; i < numberOfTimes; i++) {
    digitalWrite(pin, HIGH);
    delay(delayTime);
    digitalWrite(pin, LOW);
    delay(delayTime);
  }
}

// NOTE: This functions isnt used, might be wont
/**
 * * Copies a portion of the sensor data array into the sample feature array.
 *
 * @param featuresLength the length of the feature array
 * @param sensorValues the number of sensor values per feature
 * @param bufferLength the length of the buffer array
 * @param sensorData the array containing the sensor data
 * @param features the array to store the sampled feature
 *
 * @throws ErrorType if an error occurs during the copy process
 */
void getFeatures(int featuresLength, int sensorValues, int bufferLength,
                 float *sensorData, float *features) {

  float tempBuffer[featuresLength * sensorValues]; // temporary buffer
  memcpy(tempBuffer, sensorData, featuresLength * sensorValues * sizeof(float));

  for (int i = 0; i < (featuresLength * sensorValues) - 1; i++) {
    features[i] = tempBuffer[i + (bufferLength * sensorValues)];
  }
}

/**
 * Adding sensor reading to the buffer for inference
 *
 * @param AccX the x-axis acceleration
 * @param AccY the y-axis acceleration
 * @param AccZ the z-axis acceleration
 * @param GyroX the x-axis rotation
 * @param GyroY the y-axis rotation
 * @param GyroZ the z-axis rotation
 */
void addSensorData(float AccX, float AccY, float AccZ, float GyroX, float GyroY,
                   float GyroZ) {
  // Insert new data
  features[endIndex] = AccX;
  features[(endIndex + 1) % featuresLength] = AccY;
  features[(endIndex + 2) % featuresLength] = AccZ;
  features[(endIndex + 3) % featuresLength] = GyroX;
  features[(endIndex + 4) % featuresLength] = GyroY;
  features[(endIndex + 5) % featuresLength] = GyroZ;

  // Update endIndex and startIndex
  endIndex = (endIndex + 6) % featuresLength;
  if (endIndex == startIndex) {
    // if the buffer is full, set the flag
    featuresReady = true;
  }
}

/**
 * @brief      Copy raw feature data in out_ptr
 *             Function called by inference library
 *
 * @param[in]  offset   The offset
 * @param[in]  length   The length
 * @param      out_ptr  The out pointer
 *
 * @return     0
 */
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
  memcpy(out_ptr, features + offset, length * sizeof(float));
  return 0;
}

void process_inference_result(ei_impulse_result_t result);

void setup() {
  Serial.begin(115200);

  // * set pin mode for I/Os
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // * Try to initialize the MPU6050 sensor
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      digitalWrite(LED2, HIGH);
      delay(100);
      digitalWrite(LED2, LOW);
      delay(100);
    }
  }
  Serial.println("MPU6050 Found!");

  // * MPU6050 range setup
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // set accelerometer range to
                                                // +-8G (options: 2, 4, 8, 16)
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);    // set gyro range to +- 500 deg/s
                                              // (options: 250, 500, 1000, 2000)
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // set filter bandwidth to 21 Hz
  delay(100);

  // * Blink LED2 iteratively 3 times
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED2, HIGH);
    delay(200);
    digitalWrite(LED2, LOW);
    delay(200);
  }

  delay(2000); // placeholder delay
}

void loop() {
  // * Get new sensor events with the readings
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // * Variables of the IMU and Millis
  float myTime = millis(); // time

  float AccX = a.acceleration.x;
  float AccY = a.acceleration.y;
  float AccZ = a.acceleration.z;

  float GyroX = g.gyro.x; // rotation
  float GyroY = g.gyro.y;
  float GyroZ = g.gyro.z;

  // float AccX = a.acceleration.x * G2MS2;
  // float AccY = a.acceleration.y * G2MS2;
  // float AccZ = a.acceleration.z * G2MS2;
  //
  // float GyroX = g.gyro.x; // rotation
  // float GyroY = g.gyro.y;
  // float GyroZ = g.gyro.z;

  float TempC = temp.temperature; // sensor temperature

  // add the sensor reading to the buffer
  addSensorData(AccX, AccY, AccZ, GyroX, GyroY, GyroZ);

  if (featuresReady == true) {
    // * Values needed by the model
    ei_impulse_result_t result = {0};
    // the features are stored into flash, and we don't want to load everything
    // into RAM
    signal_t features_signal;
    features_signal.total_length = sizeof(features) / sizeof(features[0]);
    features_signal.get_data = &raw_feature_get_data;

    // check the size of the feature buffer
    if (sizeof(features) / sizeof(float) !=
        EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
      ei_printf("The size of your 'features' array is not correct. Expected "
                "%lu items, but had %lu\n",
                EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE,
                sizeof(features) / sizeof(float));
      delay(1000);
      return;
    }

    // invoke the impulse
    EI_IMPULSE_ERROR res =
        run_classifier(&features_signal, &result, false /* debug */);
    if (res != EI_IMPULSE_OK) {
      ei_printf("ERR: Failed to run classifier (%d)\n", res);
      return;
    }

    // print inference return code
    ei_printf("run_classifier returned: %d\r\n", res); // if 0 then success
    process_inference_result(result); // TODO: remix this function

    // emptying the features buffer and resets the flag
    memset(features, 0, featuresLength * sensorValues * sizeof(float));
    featuresReady = false;
  }

#ifdef DEBUG_RUN
  Serial.print("Time: "); // Time
  Serial.println(myTime);

  Serial.print("Acceleration X: "); // Acceleration
  Serial.print(AccX);
  Serial.print(", Y: ");
  Serial.print(AccY);
  Serial.print(", Z: ");
  Serial.print(AccZ);
  Serial.println(" m/s^2");

  Serial.print("Rotation X: "); // Gyro
  Serial.print(GyroX);
  Serial.print(", Y: ");
  Serial.print(GyroY);
  Serial.print(", Z: ");
  Serial.print(GyroZ);
  Serial.println(" rad/s");

  Serial.print("Temperature: "); // Temp
  Serial.print(TempC);
  Serial.println(" degC");

  if (featuresReady == true) {
    Serial.print("Buffer contents: ");
    int i = startIndex;
    do {
      Serial.print(sensorData[i]);
      Serial.print(", ");
      i = (i + 1) % sensorDataSize;
    } while (i != endIndex);
    Serial.println();
    Serial.println("------------------------");
  } else {
    Serial.println("------------------------");
  }
#endif

  // * Wait for some time before the next reading
  delay(freq);
}

// NOTE: DEAL WITH THE RESULTS HERE INSTEAD
void process_inference_result(ei_impulse_result_t result) {

  /*
  Categories[0] = fall
  Categories[1] = sit
  Categories[2] = sleep
  Categories[3] = slipped
  Categories[4] = tripped
  Categories[5] = walk
  */

  // Print how long it took to perform inference
  ei_printf("Timing: DSP %d ms, inference %d ms, anomaly %d ms\r\n",
            result.timing.dsp, result.timing.classification,
            result.timing.anomaly);

  ei_printf("Predictions:\r\n");
  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    ei_printf("  %s: ", ei_classifier_inferencing_categories[i]);
    ei_printf("%.5f\r\n", result.classification[i].value);
  }

  // deal with the results of the model inference
  if (result.classification[0].value >= confidenceThreshold ||
      result.classification[3].value >= confidenceThreshold ||
      result.classification[4].value >= confidenceThreshold) {
    digitalWrite(LED1, HIGH);
    // digitalWrite(LED2, LOW);
    // blinkLoop(150, 10, 2); // 2 is LED1
  } else if (result.classification[1].value >= confidenceThreshold ||
             result.classification[2].value >= confidenceThreshold ||
             result.classification[5].value >= confidenceThreshold) {
    digitalWrite(LED1, LOW);
    // digitalWrite(LED2, HIGH);
    // blinkLoop(150, 10, 4); // 4 is LED2
  }
}
