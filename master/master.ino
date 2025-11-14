#include <pthread.h>
#include "DHT.h"

/* ######### Defines ######### */
#define CIRCUIT_REFERENCE 3.3
#define ADC_RESOLUTION 4095.0
#define READING_TIME_S 3000

#define DHT_PIN 32
#define DHT_TYPE DHT11

#define LDR_PIN 33
#define LDR_A 50000.0    // constante de calibração
#define LDR_B 0.7        // constante de calibração
#define LDR_RFIXED 10000.0

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
  const int samples = 8;
  long sumAdcValues = 0;

  for(int i = 0; i < samples; i++){
    sumAdcValues += analogRead(LDR_PIN);
    delay(5);
  }
  
  float adcValue = (float) sumAdcValues/samples;
  float voltage = (adcValue * CIRCUIT_REFERENCE)/ADC_RESOLUTION;
  if (voltage <= 0.0001) voltage = 0.0001;
  float resistance = LDR_RFIXED * (CIRCUIT_REFERENCE/(voltage - 1));
  float lux = pow((LDR_A/LDR_RFIXED), (1.0/LDR_B));

  Serial.print("LDR = ADC = ");
  Serial.println(adcValue);
  Serial.print("LDR = Voltage = ");
  Serial.println(voltage);
  Serial.print("LDR = Resistence = ");
  Serial.println(resistance);

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
