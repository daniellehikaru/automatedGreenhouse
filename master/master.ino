#include <pthread.h>
#include "DHT.h"
#include <WiFi.h>
#include <PubSubClient.h>

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

typedef struct {
    bool airing;
    bool watering;
    bool lightning;
} Systems_t;


/* ######### Global Variables/Constants ######### */
/* ---------- CONFIG WIFI Constants ---------- */
const char* ssid = "POCO X3 Pro";
const char* password = "12345678";

/* ---------- CONFIG MQTT (THINGSBOARD) ---------- */
const char* mqttServer = "mqtt.thingsboard.cloud";
const int mqttPort = 1883;
const char* tbToken = "SistemasEmbarcados";
const char* tbPassword = "grauB123";
const char* tbTopic = "v1/devices/me/telemetry";

/* ---------- Variables ---------- */
Sensors_t greenhouseSensors = {0};
Systems_t greenhouseSystems = {0};
DHT dhtSensor(DHT_PIN, DHT_TYPE);
WiFiClient espWifiClient;
PubSubClient mqttClient(espWifiClient);

/* ######### Private functions ######### */
pthread_mutex_t sensorMutex = PTHREAD_MUTEX_INITIALIZER;

/* ######### Private functions ######### */
static void checkAiringSystem(){
  // Check Temperature
  if(greenhouseSensors.temperature > TEMPERATURE_REFERENCE){
    digitalWrite(OUT_AIRING_PIN, HIGH);
    greenhouseSystems.airing = true;
  }
  else{
    digitalWrite(OUT_AIRING_PIN, LOW);
    greenhouseSystems.airing = false;
  }
}

static void checkWateringSystem(){
  // Check Luminosity
  if(greenhouseSensors.humidity < HUMIDITY_REFERENCE){
    digitalWrite(OUT_WATERING_PIN, HIGH);
    greenhouseSystems.watering = true;
  }
  else{
    digitalWrite(OUT_WATERING_PIN, LOW);
    greenhouseSystems.watering = false;
  }
}

static void checkLightningSystem(){
  // Check Luminosity
  if(greenhouseSensors.luminosity < LUMINOSITY_REFERENCE){
    digitalWrite(OUT_LIGHTNING_PIN, HIGH);
    greenhouseSystems.lightning = true;
  }
  else{
    digitalWrite(OUT_LIGHTNING_PIN, LOW);
    greenhouseSystems.lightning = false;
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

  return voltage;
}

static String buildPayload(){
  String payload = String("{\"temperatura\":") + String(greenhouseSensors.temperature, 2)
                + String(",\"umidade\":") + String(greenhouseSensors.humidity, 2)
                + String(",\"luminosidade\":") + String(greenhouseSensors.luminosity, 2)
                + String(",\"ventilacao\":") + (greenhouseSystems.airing ? "true" : "false")
                + String(",\"irrigacao\":") + (greenhouseSystems.watering ? "true" : "false")
                + String(",\"iluminacao\":") + (greenhouseSystems.lightning ? "true" : "false")
                + String("}");

  return payload;
} 

/* ######### Threads Definition ######### */
void* t_readings(void *arg) {
  while(1){
      pthread_mutex_lock(&sensorMutex);
      
      greenhouseSensors.temperature = dhtSensor.readTemperature();
      greenhouseSensors.humidity = dhtSensor.readHumidity();
      greenhouseSensors.luminosity = readLuminosity();

      if (isnan(greenhouseSensors.temperature) || isnan(greenhouseSensors.humidity)) {
        Serial.println("Falha na leitura do sensor DHT11!");
        break;
      }

      checkAiringSystem();
      checkWateringSystem();
      checkLightningSystem();

      pthread_mutex_unlock(&sensorMutex);

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

      usleep((useconds_t)READING_TIME_S * 1000);
  }

  return NULL;
}

void* t_iot(void *arg){
  mqttClient.setServer(mqttServer, mqttPort);

  while(1) {
    /* ----------------- WIFI ----------------- */
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Conectando...");
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);

      int tentativas = 0;
      while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
        Serial.print(".");
        usleep(300 * 1000);
        tentativas++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] Conectado: " + WiFi.localIP().toString());
      } else {
        Serial.println("\n[WiFi] Falhou!");
        usleep(2000 * 1000);
        continue;
      }
    }

    /* ----------------- MQTT ----------------- */
    if (!mqttClient.connected()) {
      Serial.println("[MQTT] Conectando ao servidor...");
      if (mqttClient.connect("teste", tbToken, tbPassword)) {
        Serial.println("[MQTT] Conectado!");
      } else {
        Serial.println("[MQTT] Falhou ao conectar. Tentando novamente...");
        usleep(2000 * 1000);
        continue;
      }
    }

    pthread_mutex_lock(&sensorMutex);
    String payload = buildPayload();
    pthread_mutex_unlock(&sensorMutex);

    if (mqttClient.publish(tbTopic, payload.c_str())) {
      Serial.println("[MQTT] Enviado -> " + payload);
    } else {
      Serial.println("[MQTT] Falha ao publicar!");
    }

    mqttClient.loop();
    usleep(6000 * 1000);
  }

  return NULL;
}


/* #########  Setup ######### */
void setup() {
  // Set main theards
  pthread_t thread1, thread2;

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
  if(!pthread_create(&thread1, NULL, &t_readings, NULL)){
    Serial.print("### Erro ao criar thread readings");
  }else{
    pthread_detach(thread1);
  }

  if (pthread_create(&thread2, NULL, &t_iot, NULL)) {
    Serial.print("### Erro ao criar thread iot");
  } else {
    pthread_detach(thread2);
  }

}

void loop() {
  
}
