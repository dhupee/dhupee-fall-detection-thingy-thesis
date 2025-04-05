/**
 * Made by Dhupee
 *
 * This is a firmware code for an ESP32 board with an MPU6050 sensor to collect
 *and store accelerometer and gyroscope data on an SD card using SPI SD card
 *module.
 *
 * This code is a backup to an original firmware due to problem either in the
 *server or my network If you somehow use this(WHY?) and have any question,
 *contact me: https://twitter.com/dhupee_haj
 *
 * This code is written and used in PlatformIO, but can be used for Arduino IDE
 *with few modification
 **/

#include "esp32-hal-gpio.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

// #define DRY_RUN // if this enabled it wont try to connect with SD Card module
#define DEBUG_RUN // if this enable it will print the results

// Assign IMU
Adafruit_MPU6050 mpu;

// Pin Assign
const int BUTTON = 12; // Button for toggle
const int LED1 = 2;    // LED for send toggle notification
const int LED2 = 4;    // LED for wether the ESP is ready

// Variables
int freq = 5; // how delay on each loop, smaller the faster, in miliseconds
volatile bool sendToggle = true; // toggle for send the data to SD card
unsigned long myTime;            // Variable for millis
int fileCounter = 1;

// Define the SPI pins for the microSD card module
#define SD_CS_PIN 5

void checkSwitch() {
  // Toggle LED if button pressed
  if (digitalRead(BUTTON) == LOW) {
    sendToggle = !sendToggle;       // Change state of toggle
    digitalWrite(LED1, sendToggle); // The LED state is changed too
  }
}

void blinkLoop() {
  digitalWrite(LED2, HIGH);
  delay(100);
  digitalWrite(LED2, LOW);
  delay(100);
}

void setup() {
  Serial.begin(115200);

  // set pin mode for I/Os
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // Try to initialize the MPU6050 sensor
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      blinkLoop();
    }
  }
  Serial.println("MPU6050 Found!");

  // MPU6050 range setup
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // set accelerometer range to
                                                // +-8G (options: 2, 4, 8, 16)
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);    // set gyro range to +- 500 deg/s
                                              // (options: 250, 500, 1000, 2000)
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // set filter bandwidth to 21 Hz
  delay(100);

#ifndef DRY_RUN
  // Initialize the microSD card
  SD.begin(SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Failed to initialize SD card");
    while (1) {
      blinkLoop();
    }
  }

  // Check if data1.csv already exists
  String filename = "/data1.csv";
  while (SD.exists(filename)) {
    // Increment the file counter
    fileCounter++;
    filename = "/data" + String(fileCounter) + ".csv";
  }

  // Create a new file for data storage
  File dataFile;
  bool fileCreated = false;
  while (!fileCreated) {
    dataFile = SD.open(filename, FILE_WRITE);
    if (dataFile) {
      dataFile.println("Time, Accelerometer X, Accelerometer Y, Accelerometer "
                       "Z, Gyroscope X, Gyroscope Y, Gyroscope Z");
      dataFile.close();
      Serial.println(filename + " file created");
      fileCreated = true;
    } else {
      Serial.println("Error creating data file");
      // Increment the file counter and try with the new filename
      fileCounter++;
      filename = "/data" + String(fileCounter) + ".csv";
    }
  }
#endif

  // Blink LED2 iteratively 3 times
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED2, HIGH);
    delay(200);
    digitalWrite(LED2, LOW);
    delay(200);
  }

  // Attach Interrupt to Interrupt Service Routine for Send Toggle
  attachInterrupt(digitalPinToInterrupt(BUTTON), checkSwitch, FALLING);
  delay(1500);
  digitalWrite(LED1, HIGH);
}

void loop() {
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Variables of the IMU and Millis
  float myTime = millis(); // time

  float AccX = a.acceleration.x; // acceleration
  float AccY = a.acceleration.y;
  float AccZ = a.acceleration.z;

  float GyroX = g.gyro.x; // rotation
  float GyroY = g.gyro.y;
  float GyroZ = g.gyro.z;

  float TempC = temp.temperature; // sensor temperature

#ifndef DRY_RUN
  if (sendToggle == true) {
    // Open the data file in append mode
    String filename = "/data" + String(fileCounter) + ".csv";
    File dataFile = SD.open(filename, FILE_APPEND);
    if (dataFile) {
      // Write the sensor data and timestamp to the file
      dataFile.print(myTime);
      dataFile.print(", ");
      dataFile.print(AccX);
      dataFile.print(", ");
      dataFile.print(AccY);
      dataFile.print(", ");
      dataFile.print(AccZ);
      dataFile.print(", ");
      dataFile.print(GyroX);
      dataFile.print(", ");
      dataFile.print(GyroY);
      dataFile.print(", ");
      dataFile.println(GyroZ);
      dataFile.close();
    } else {
      Serial.println("Error opening data file");
    }
  }
#endif

#ifdef DEBUG_RUN
  // if this disabled it won't send the serial print in serial monitor
  if (sendToggle == true) {
    Serial.println("Sending to data" + String(fileCounter) + ".csv");
  } else {
    Serial.println("Not sending");
  }

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

  Serial.println("------------------------");
#endif

  // Wait for some time before the next reading
  delay(freq);
}
