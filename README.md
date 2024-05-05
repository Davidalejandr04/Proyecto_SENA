## Titulo del proyecto: 'Implementación de una red de nodos sensores para medición de variables físicas en invernadero en el Servicio Nacional de Aprendizaje SENA, San Juan de Pasto'

En este repositorio se encuentra los códigos que se trabajó durante la pasantía en el Servicio Nacional de Aprendizaje SENA.
- En la carpeta AnalogReadSerial, se encuentra el código para tomar lectura de un sensor analógico (capacitive moisture) en el pin A0 de la placa Arduino nano,
  esto con el fin de realizar la calibración del sensor, éste código se obtuvo a partir de los ejemplos que se indica en el entorno de programación Arduino IDE.
- En la carpeta LoRaReceiver, se encuentra el código para recibir el mensaje por medio de LoRa del transmisor éste código se obtuvo a partir de los ejemplos que se indica en el entorno de programación Arduino IDE descargando la librería LoRa.
- En la carpeta LoRaReceiver, se encuentra el código para enviar el mensaje por medio de LoRa hacia el receptor éste código se obtuvo a partir de los ejemplos que se indica en el entorno de programación Arduino IDE descargando la librería LoRa.
- En las carpetas node1.1 y node 2.2, se encuentra el código de los nodos sensores que se encargará de adquirir los datos a partir de los sensores y enviar por medio de LoRa hacia el recpetor (Raspberry pi 3 B+).
- En el archivo raspberry_rx.py, se encuentra el código de la raspberry que se encargará de recibir los datos enviados por los nodos sensores y posteriormente subir a una base de datos uilizando MySQL.
- Se encuentra el informe final del proyecto donde se indica el proceso.
