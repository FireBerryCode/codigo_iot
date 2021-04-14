#!/usr/bin/env python

#
# Example using Dynamic Payloads
# 
#  This is an example of how to use payloads of a varying (dynamic) size.
# 

from __future__ import print_function
import time
from RF24 import *
import RPi.GPIO as GPIO
from struct import *
import platform
import datetime
from iotclient import get_client


########### USER CONFIGURATION ###########
# See https://github.com/TMRh20/RF24/blob/master/pyRF24/readme.md
# Radio CE Pin, CSN Pin, SPI Speed
# CE Pin uses GPIO number with BCM and SPIDEV drivers, other platforms use their own pin numbering
# CS Pin addresses the SPI bus number at /dev/spidev<a>.<b>
# ie: RF24 radio(<ce_pin>, <a>*10+<b>); spidev1.0 is 10, spidev1.1 is 11 etc..

# Generic:
radio = RF24(22,0);

# RPi Alternate, with SPIDEV - Note: Edit RF24/arch/BBB/spi.cpp and  set 'this->device = "/dev/spidev0.0";;' or as listed in /dev
#radio = RF24(22, 0);


# Setup for connected IRQ pin, GPIO 24 on RPi B+; uncomment to activate
#irq_gpio_pin = 24

##########################################
def try_read_data(channel=0):
  while True:
    if radio.available():
        while radio.available():
            len = radio.getDynamicPayloadSize()
            receive_payload = radio.read(len)
            hum = unpack('f', receive_payload[0:4])[0]
            temp = unpack('f', receive_payload[4:8])[0]
            flame = unpack('f', receive_payload[8:12])[0]
            mq7 = unpack('f', receive_payload[12:16])
            mq2 = unpack('f', receive_payload[16:20])
            tsl = unpack('f', receive_payload[20:24])
            
            print("Humedad: ", hum, " %")
            print("Temperatura: ", temp, " ºC")
            print("Llama: ", flame)
            print("MQ-7: ", mq7)
            print("MQ-2: ", mq2)
            print("TSL: ", tsl)
            # First, stop listening so we can talk
            radio.stopListening()

            # Send the final one back. Devolver un hash
            radio.write(receive_payload)
            print('Sent response.')

            # Now, resume listening so we catch the next packets.
            radio.startListening()
            payload = {"Humedad": hum, "Temperatura": temp, "Llama": flame, "MQ-7": mq7, "MQ-2": mq2, "TSL": tsl}

        return str(payload)


pipes = [0xF0F0F0F0E1, 0xF0F0F0F0D2]
min_payload_size = 4
max_payload_size = 32


radio.begin()
radio.enableDynamicPayloads()
radio.setRetries(5, 15)
radio.printDetails()

radio.openWritingPipe(pipes[1])
radio.openReadingPipe(1, pipes[0])
radio.startListening()

### CLIENTE IOTCORE ###
mqtt_topic = '/devices/my-device/gateway-telemetry'

jwt_iat = datetime.datetime.utcnow()
jwt_exp_mins = 1
client = get_client(
    "gold-braid-297420", "europe-west1", "my-registry",
    "my-device", "rsa_private.pem", "RS256",
    "roots.pem", "mqtt.googleapis.com", 8883)

# forever loop
while True:

    client.loop()
  
    payload = try_read_data()

    seconds_since_issue = (datetime.datetime.utcnow() - jwt_iat).seconds
    if seconds_since_issue > 60 * jwt_exp_mins:
        print('Refreshing token after {}s'.format(seconds_since_issue))
        jwt_iat = datetime.datetime.utcnow()
        client.loop()
        client.disconnect()
        client = get_client(
            "gold-braid-297420", "europe-west1","my-registry",
            "my-device", "rsa_private.pem", "RS256",
            "roots.pem", "mqtt.googleapis.com", 8883)

    client.publish(mqtt_topic, payload, qos=1)         
