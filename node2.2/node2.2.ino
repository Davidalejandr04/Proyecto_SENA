#include <cppQueue.h>
#include <TaskScheduler.h>
#include <SPI.h>
#include <LoRa.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include "LowPower.h"

OneWire oneWire(5);
DallasTemperature ds18b20(&oneWire);

#define SOIL_QUEUE_SIZE 5
#define DS_QUEUE_SIZE 5

cppQueue soil_queue(sizeof(float), SOIL_QUEUE_SIZE, FIFO);
cppQueue ds_queue(sizeof(float), DS_QUEUE_SIZE, FIFO);

// Definir intervalos de adquisición de datos y envío de promedio
#define DATA_INTERVAL 5000
#define SEND_INTERVAL 30000

// Variable para almacenar la solicitud recibida
int request = 0;

// Definir objeto de TaskScheduler
Scheduler scheduler;

// Declarar funciones
void readsoil();

void readDS18B20();

void sendAverage();

void receiveRequest();

// Definir tarea para adquisición de datos
Task dataTask3(DATA_INTERVAL, TASK_FOREVER, &readDS18B20, &scheduler, true);
Task dataTask4(DATA_INTERVAL, TASK_FOREVER, &readsoil, &scheduler, true);

// Definir tarea para enviar promedio por LoRa
Task sendTask(SEND_INTERVAL, TASK_FOREVER, &sendAverage, &scheduler, true);

// Definir tarea para recibir solicitudes por LoRa
Task receiveTask(0, TASK_FOREVER, &receiveRequest, &scheduler, true);

void setup() {
  Serial.begin(9600);

  // frecuencia LoRa
  LoRa.begin(915E6);
  LoRa.setTxPower(20);  

  // Añadir tareas
  scheduler.addTask(dataTask3);
  scheduler.addTask(dataTask4);
  scheduler.addTask(sendTask);
  scheduler.addTask(receiveTask);

  // Habilitar tareas
  dataTask3.enable();
  dataTask4.enable();
  sendTask.enable();
  receiveTask.enable();
}

void loop() {
  // Ejecutar tareas programadas
  scheduler.execute();
}

void readDS18B20() {
  ds18b20.requestTemperatures();
  float temperature = ds18b20.getTempCByIndex(0);
  if (isnan(temperature)) {
    Serial.println("Error al leer DS18B20");
    return;
  }

  ds_queue.push(&temperature);
}

void readsoil()
{
  int moisture = analogRead(A0);
  float soil = map(moisture, 476, 236, 0, 100);
  if (soil < 0)
  {
    soil = 0;
  }
  else if (soil > 100)
  {
    soil = 100;
  }
  soil_queue.push(&soil);  

}

void sendAverage() {

  // verificacion si hay suficientes datos en la cola
  if (ds_queue.getCount() < DS_QUEUE_SIZE) {
    return;
  }

  if (soil_queue.getCount() < SOIL_QUEUE_SIZE) {
    return;
  }

  //leer valores de las colas y acumular para promedio

  float ds_sum = 0.0;
  
  for(int i = 0; i < DS_QUEUE_SIZE; i++){
    float value;
    if (ds_queue.pop(&value)){
      ds_sum += value;    
    }else{
      break;
    }
  }
  
  float soil_sum = 0.0;
  for (int i = 0; i < SOIL_QUEUE_SIZE; i++) {
    float value;
    if (soil_queue.pop(&value)) {
    soil_sum += value;
  } else {
    break;
  }
}

// Calcular promedio
float soil_avg = soil_sum / (SOIL_QUEUE_SIZE);
float ds_avg = ds_sum / (DS_QUEUE_SIZE);
  // Crear el objeto JSON
  DynamicJsonDocument jsonBuffer(256);
  JsonObject root = jsonBuffer.to<JsonObject>();
  JsonObject sensor = root.createNestedObject("SENSOR");
  //JsonObject ds = root.createNestedObject("DS");
  root["nodo"] = 2;
  sensor["humedad"] = soil_avg;
  sensor["temperatura"]= ds_avg;

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
  while (request != 2) {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      while (LoRa.available()) {
        request = LoRa.read();
      }
    }
  }

    // Procesar la solicitud recibida
    if (request == 2) {
      // Si la solicitud es 2, responder con los datos solicitados
      sendAverage();
      delay(200);
      request = 0;
    }
  }
