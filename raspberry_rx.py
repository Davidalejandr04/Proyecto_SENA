import json
import adafruit_rfm9x
import busio
from digitalio import DigitalInOut, Direction, Pull 
import board
import MySQLdb
import datetime

cs = DigitalInOut(board.D25)
reset = DigitalInOut(board.D17)
spi = busio.SPI(board.SCK, MOSI=board.MOSI, MISO=board.MISO)
rfm9x = adafruit_rfm9x.RFM9x(spi, cs, reset, 915.0)

con = MySQLdb.connect(host='localhost',
                      user='raspberry',
                      password='raspberry',
                      database='monitoreo')

cur = con.cursor()

cur.execute(''' CREATE TABLE IF NOT EXISTS datos (hora TIMESTAMP, 
                                                     nodo INTEGER, 
                                                     tem_nd1 FLOAT, 
                                                     hum_nd1 FLOAT, 
                                                     hum_nd2 FLOAT,
                                                     tem_nd2 FLOAT )''')

cont = 0

while True:
    try:
                
        rfm9x.send(bytes([1]))
        
        # Esperar la respuesta
        packet1 = rfm9x.receive()
        if packet1 is not None:
            packet_text1 = str(packet1, "utf-8")
            cont += 1
            
            #print("Datos recibidos:", packet_text1)
            try:
                json_data = json.loads(packet_text1)
                sender_id = json_data['nodo']
                data = json_data['DHT11']
                temperature = data['temperatura']
                humidity = data['humedad']
                print("NUM: ",cont)
                print("Nodo:", sender_id)
                print("Temperatura:", temperature)
                print("Humedad:", humidity)
                cur.execute("INSERT INTO datos (nodo, tem_nd1, hum_nd1) VALUES (%s,%s,%s)",(sender_id,temperature,humidity))
                con.commit()
            except json.JSONDecodeError:
                print("Error al decodificar JSON")
                with open("respaldo.json", "a") as archivo_respaldo:
                    archivo_respaldo.write(packet_text1 + "\n")
    
    except UnicodeDecodeError:
        print("Error al decodificar el paquete 1")
        with open("respaldo.json", "a") as archivo_respaldo:
            archivo_respaldo.write("Error al decodificar el paquete 1 \n")
        continue
    
    try:
        # Solicitar datos al nodo 2 enviando una solicitud
        rfm9x.send(bytes([2]))
        
        # Esperar la respuesta
        packet2 = rfm9x.receive()
        if packet2 is not None:
            packet_text2 = str(packet2, "utf-8")
            
            #print("Datos recibidos:", packet_text2)
            try:
                json_data1 = json.loads(packet_text2)
                sender_id = json_data1['nodo']
                data1 = json_data1['SENSOR']
                hum_soil = data1['humedad']
                tem_ds = data1['temperatura']
                print("NUM: ",cont)
                print("Nodo:", sender_id)
                print("Humedad:", hum_soil)
                print("Temperatura:", tem_ds)
                cur.execute("INSERT INTO datos (nodo, hum_nd2, tem_nd2) VALUES (%s,%s,%s)",(sender_id,hum_soil,tem_ds))
                con.commit()
            except json.JSONDecodeError:
                print("Error al decodificar JSON")
                with open("respaldo.json", "a") as archivo_respaldo:
                    archivo_respaldo.write(packet_text2 + "\n")
    
    except UnicodeDecodeError:
        print("Error al decodificar el paquete 2")
        with open("respaldo.json", "a") as archivo_respaldo:
            archivo_respaldo.write("Error al decodificar el paquete 2 \n")
        continue
