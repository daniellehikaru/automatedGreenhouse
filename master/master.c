#include <pthread.h>
#include "DHT.h"

/* ######### Defines ######### */
#define CIRCUIT_REFERENCE 5
#define ADC_RESOLUTION 4095.0
#define READING_TIME_S 3000

#define DHT_PIN 12
#define DHT_TYPE DHT22          // Change to DHT11 in real system

#define LDR_PIN 34
#define LDR_GAMMA 0.7
#define LDR_RL10 50

/* ######### Types Definition ######### */
typedef struct {
    float temperature;
    float humidity;
    float luminosity;
} Sensors_t;

/* ######### Global Variables ######### */
Sensors_t greenhouseSensors = {0};
DHT dhtSensor(DHT_PIN, DHT_TYPE);

/* ######### Private functions ######### */

// Will be necessary to adjust this method depending the LDR that we use (is implemented now to the LDR of Wokwi simulator)
static float readLuminosity(){
  int ldrAdcValue = analogRead(LDR_PIN);
  float voltage = ldrAdcValue / ADC_RESOLUTION * CIRCUIT_REFERENCE;
  float resistance = 2000 * voltage / (1 - voltage / CIRCUIT_REFERENCE);
  float lux = pow(LDR_RL10 * 1e3 * pow(10, LDR_GAMMA) / resistance, (1 / LDR_GAMMA));

  return lux;
}

/* ######### Threads Definition ######### */
void* t_readings(void *arg) {
  while(1){
      // Read Temperature
      greenhouseSensors.temperature = dhtSensor.readTemperature();
      // Read Humidity
      greenhouseSensors.humidity = dhtSensor.readHumidity();
      // Read Luminosity
      greenhouseSensors.luminosity = readLuminosity();

      // Checking DHT reading failure
      if (isnan(greenhouseSensors.temperature) || isnan(greenhouseSensors.humidity)) {
        Serial.println("Falha na leitura do sensor DHT11!");
        break;
      }

      Serial.print("Reading Temperature = ");
      Serial.print(greenhouseSensors.temperature);
      Serial.println(" °C");
      Serial.print("Reading Humidity = ");
      Serial.print(greenhouseSensors.humidity);
      Serial.println(" %\t");
      Serial.print("Reading Luminosity = ");
      Serial.print(greenhouseSensors.luminosity);
      Serial.println(" lux");

      delay(READING_TIME_S);
  }
}

/* #########  Setup ######### */
void setup() {
  // Set main theards
  pthread_t thread1;

  // Debug Serial:
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Start DHT
  dhtSensor.begin();

  // Create threads
  pthread_create(&thread1, NULL, &t_readings, NULL);

  // Start threads
  pthread_join(thread1, NULL);

}

void loop() {
  
}
