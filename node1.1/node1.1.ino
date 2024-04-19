#include "DHT.h"
#include <cppQueue.h>
#include <TaskScheduler.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include "LowPower.h"


// Definir pines y configuración de los sensores
#define DHTPIN 3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Definir tamaños de cola para el promedio
#define DHT_QUEUE_SIZE 10

cppQueue dht_queue(sizeof(float), DHT_QUEUE_SIZE, FIFO);

// Definir intervalos de adquisición de datos y envío de promedio
#define DATA_INTERVAL 5000
#define SEND_INTERVAL 25000

// Variable para almacenar la solicitud recibida
int request = 0;

// Definir objeto de TaskScheduler
Scheduler scheduler;

// Declarar funciones
void readDHT();

void sendAverage();

void receiveRequest();

// Definir tarea para adquisición de datos
Task dataTask(DATA_INTERVAL, TASK_FOREVER, &readDHT, &scheduler, true);

// Definir tarea para enviar promedio por LoRa
Task sendTask(SEND_INTERVAL, TASK_FOREVER, &sendAverage, &scheduler, true);

// Definir tarea para recibir solicitudes por LoRa
Task receiveTask(0, TASK_FOREVER, &receiveRequest, &scheduler, true);

void setup() {
  Serial.begin(9600);

  // frecuencia LoRa
  LoRa.begin(915E6);

  // Inicializar sensores
  dht.begin();

  // Añadir tareas
  scheduler.addTask(dataTask);
  scheduler.addTask(sendTask);
  scheduler.addTask(receiveTask);

  // Habilitar tareas
  dataTask.enable();
  sendTask.enable();
  receiveTask.enable();
}

void loop() {

  // Ejecutar tareas programadas
  scheduler.execute();


}

void readDHT() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Error al leer DHT11");
    return;
  }

  dht_queue.push(&temperature);
  dht_queue.push(&humidity);
}

void sendAverage() {
  // Verificar si hay suficientes datos en la cola
  if (dht_queue.getCount() < DHT_QUEUE_SIZE) {
    return;
  }

  // Leer valores de las colas y acumular para el promedio
  float dht_temp_sum = 0, dht_hum_sum = 0;
  for (int i = 0; i < DHT_QUEUE_SIZE; i++) {
    float value;
    if (dht_queue.pop(&value)) {
      dht_temp_sum += value;
      if (dht_queue.pop(&value)) {
        dht_hum_sum += value;
      } else {
        break;
      }
    } else {
      break;
    }
  }

  // Calcular promedios
  float dht_temp_avg = dht_temp_sum / (DHT_QUEUE_SIZE/2);
  float dht_hum_avg = dht_hum_sum / (DHT_QUEUE_SIZE/2);

  // Crear el objeto JSON
  DynamicJsonDocument jsonBuffer(256);
  JsonObject root = jsonBuffer.to<JsonObject>();
  JsonObject dht = root.createNestedObject("DHT11");
  root["nodo"] = 1;
  dht["temperatura"] = dht_temp_avg;
  dht["humedad"] = dht_hum_avg;



  String jsonStr;
  serializeJson(root, jsonStr);
  Serial.println(jsonStr);
  // Enviar datos por LoRa
  LoRa.beginPacket();
  LoRa.print("\n\n   ");
  LoRa.println(jsonStr);
  LoRa.endPacket();

  for (int i = 0; i < 1200; i++) {
    LowPower.idle(SLEEP_1S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                  SPI_OFF, USART0_OFF, TWI_OFF);
  }

}

void receiveRequest() {
  // Esperar hasta que request sea igual a 2
  while (request != 1) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      while (LoRa.available()) {
        request = LoRa.read();
      }
    }
  }

  // Procesar la solicitud recibida
  if (request == 1) {
    sendAverage();
    delay(200);
    request = 0;
  }

}
