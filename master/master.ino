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

#define OUT_LIGHTNING_PIN 12
#define OUT_WATERING_PIN 13
#define OUT_AIRING_PIN 14

#define LUMINOSITY_REFERENCE 2.00
#define HUMIDITY_REFERENCE 45.00
#define TEMPERATURE_REFERENCE 26.00

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

static void checkAiringSystem(){
  // Check Temperature
  if(greenhouseSensors.temperature > TEMPERATURE_REFERENCE){
    digitalWrite(OUT_AIRING_PIN, HIGH);
  }
  else{
    digitalWrite(OUT_AIRING_PIN, LOW);
  }
}

static void checkWateringSystem(){
  // Check Luminosity
  if(greenhouseSensors.humidity < HUMIDITY_REFERENCE){
    digitalWrite(OUT_WATERING_PIN, HIGH);
  }
  else{
    digitalWrite(OUT_WATERING_PIN, LOW);
  }
}

static void checkLightningSystem(){
  // Check Luminosity
  if(greenhouseSensors.luminosity < LUMINOSITY_REFERENCE){
    digitalWrite(OUT_LIGHTNING_PIN, HIGH);
  }
  else{
    digitalWrite(OUT_LIGHTNING_PIN, LOW);
  }
}

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

  //float resistance = LDR_RFIXED * (CIRCUIT_REFERENCE/(voltage - 1));
  //float lux = pow((LDR_A/LDR_RFIXED), (1.0/LDR_B));

  return voltage;
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

      // Check values to set output systems
      checkAiringSystem();
      checkWateringSystem();
      checkLightningSystem();

      // Debug
      Serial.println("#########################");
      Serial.print("Reading Temperature = ");
      Serial.print(greenhouseSensors.temperature);
      Serial.println(" °C");
      Serial.print("Reading Humidity = ");
      Serial.print(greenhouseSensors.humidity);
      Serial.println(" %\t");
      Serial.print("Reading Luminosity = ");
      Serial.print(greenhouseSensors.luminosity);
      Serial.println(" lux");
      Serial.println("#########################");

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

  // Set outputs pins
  pinMode(OUT_LIGHTNING_PIN, OUTPUT);
  pinMode(OUT_WATERING_PIN, OUTPUT);
  pinMode(OUT_AIRING_PIN, OUTPUT);

  // Set initial output states
  digitalWrite(OUT_AIRING_PIN, LOW);
  digitalWrite(OUT_WATERING_PIN, LOW);
  digitalWrite(OUT_LIGHTNING_PIN, LOW);

  // Start DHT
  dhtSensor.begin();

  // Create threads
  pthread_create(&thread1, NULL, &t_readings, NULL);

  // Start threads
  pthread_join(thread1, NULL);

}

void loop() {
  
}
