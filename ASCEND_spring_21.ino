/*
 Name:    ASCEND_S21.ino
 Created: 3/8/2021 7:21:38 PM
 Author:  Nick Blanchard

 Flight code controlling the UARIZONA SEDS payload for the Arizona Space Grant
 Spring 2021 ASCEND launch.

 Hardware:
 ATMEGA329P microcontroller
 BME680 Temp/Pressure/Humidity/Gas sensor
 MMA8451 3-axis accelerometer
 Radiation Watch Pocket Geiger
 LED / Resistor circuit

 Powered using an Adafruit Poweboost 1000c board
*/


// Libraries
#include <SD.h>                 // For writing to Adafruit Micro SD Card+ Module
#include <SPI.h>                // Hardware SPI library
#include <Wire.h>               // For I2C Communication with Arduino hardware
#include <Adafruit_MMA8451.h>   // For reading data from the Adafruit MMA8451 3-Axis Accelerometer
#include <Adafruit_Sensor.h>    // For reading data from Adafruit sensors
#include "Adafruit_BME680.h"    // For reading data from the Adafruit BME680 sensor
#include "RadiationWatch.h"     // Library to help control the Pocket Geiger

// LED pin
#define led_pin A1

// Hardware SPI
#define CS 10

// File on the SD card to store data in
File data_file;

// Get MMA, BME, and Geiger Counter objects
Adafruit_MMA8451 mma = Adafruit_MMA8451();  // I2C
Adafruit_BME680 bme;                        // I2C
RadiationWatch radiationWatch;              // Digital




void get_mma_data() {
    /* This function collects data from the MMA sensor.
   If the data is scollected without a problem, the function returns 1.
   If there is an issue while collecting data, the function return a -1.
   All data that is successfully collected is written to the MicroSD card in the format:

   x-accel, y-accel, z-accel [WITH newline character]

   */
   
    /* Get a new sensor event */
    sensors_event_t event;
    mma.getEvent(&event);

    // Write this data to the MicroSD card file
    if (data_file) {
        data_file.print(event.acceleration.x); data_file.print(", ");
        data_file.print(event.acceleration.y); data_file.print(", ");
        data_file.print(event.acceleration.z); data_file.print(", ");
    }
    else {
      digitalWrite(led_pin, LOW);
      return;
    }

    // If successful, add a delay and turn the LED on
    digitalWrite(led_pin, HIGH);
    return;
}





void get_bme_data() {
    /* This function collects data from the BME680 sensor.
    If the data is scollected without a problem, the function returns 1.
    If there is an issue while collecting data, the function return a -1.
    All data that is successfully collected is written to the MicroSD card in the format:

    temperature, pressure, humidity, gas_resistance, altitude [NO newline character]

    */
    
    // If we fail to get a reading, return -1
    if (! bme.performReading()) {
        Serial.println("Failed to perform BME reading :(");
        digitalWrite(led_pin, LOW);
        return;
    }

    // Write this data to the MicroSD card file
    if (data_file) {
        data_file.print(bme.temperature);
        data_file.print(", ");
        data_file.print(bme.pressure);
        data_file.print(", ");
        data_file.print(bme.humidity);
        data_file.print(", ");
        data_file.print(bme.gas_resistance);
        data_file.print(", ");
        data_file.print(bme.readAltitude(1013.25));
        data_file.print(", ");
    }
    else {
        digitalWrite(led_pin, LOW);
        return;
    }

    // Turn on LED if successful
    digitalWrite(led_pin, HIGH);
}





void get_geiger_data() {
  
  data_file.print(radiationWatch.uSvh()); data_file.print(", ");
  data_file.print(radiationWatch.cpm());
  Serial.println(radiationWatch.uSvh()); //DEBUG
  Serial.println(radiationWatch.cpm());  // DEBUG
}




void setup() {
    // Set up serial port
    Serial.begin(9600);
    while(!Serial);

    // Set up LED pin
    pinMode(led_pin, OUTPUT);
    pinMode(10, OUTPUT);
    digitalWrite(led_pin, HIGH);

    // Set up MMA
    while (!mma.begin()) {
        Serial.println("Couldnt start MMA");
        digitalWrite(led_pin, LOW);
        delay(500);
    }
    mma.setRange(MMA8451_RANGE_2_G);
    digitalWrite(led_pin, HIGH);

    // Set up BME 680
    while (!bme.begin()) {
        Serial.println("Couldn't start BME");
        digitalWrite(led_pin, LOW);
        delay(500);
    }
    digitalWrite(led_pin, HIGH);

    // Set up pocket geiger
    radiationWatch.setup();
    
    // Set up oversampling and filter initialization for BME
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320*C for 150 ms

    // Set up SD card
    while (!SD.begin(10)) {
      Serial.println("initialization failed!");
      digitalWrite(led_pin, LOW);
      delay(500);
    }
    Serial.println("initialization done.");

    // Open data file for writing
    data_file = SD.open("test.txt", FILE_WRITE);
    
    // If we made it to this point, the BME and MMA are successfully initialized. Turn on LED to reflect this.
    digitalWrite(led_pin, HIGH);

    // Write the format of the data to the file
    data_file.println("millis, temperature, pressure, humidity, gas_resistance, altitude, x-accel, y-accel, z-accel, uSvh, cpm");
    data_file.close();

    // Loop for the entire flight
    while(1) {
      // Open the data file for writing
      data_file = SD.open("test.txt", FILE_WRITE);

      // Print the timestamp and data
      data_file.print(millis()); data_file.print(" ");
      get_bme_data();
      get_mma_data();
      radiationWatch.loop();
      get_geiger_data();

      // Close the file before the next loop
      data_file.println("");
      data_file.close();
      delay(1000);
    }
}
