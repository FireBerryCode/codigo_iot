#include <SPI.h>
#include "RF24.h"
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"

//Declaremos los pines CE y el CSN
#define CE_PIN 9
#define CSN_PIN 53
//Declaramos los pines de los sensores
#define DHT_PIN_DATA  2
#define IRFLAME_5V_PIN_AOUT A10
#define MQ2_5V_PIN_AOUT A0
#define MQ7_5V_PIN_AOUT A3

//TODO: COMUNICACIÓN POR RADIO, AHORA MISMO NON FUNCIONA.

 
//Variable con la dirección del canal por donde se va a transmitir
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

//creamos el objeto radio (NRF24L01)
RF24 radio(CE_PIN, CSN_PIN);

//creamos el objeto dht
DHT dht(DHT_PIN_DATA, DHT22);

//creamos el objeto tsl
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); //el número es un identificador del sensor

float hum;  //Almacena el valor de la humedad
float temp; //Almacena el valor de la temperatura
int flame; //Almacena el valor del sensor de llama
int mq7; //Almacena el valor del sensor MQ-7
int mq2; //Almacena el valor del sensor MQ-2
uint16_t tsl_visible; //Almacena el valor del sensor tsl en el espectro visible
//con el tsl se pueden hacere más lecturas, ver el ejemplo tsl2591



const int max_payload_size = 32;
char receive_payload[max_payload_size + 1];

//vector con los datos a enviar
float datos[3];

void setup()
{
  //inicializamos el NhayRF24L01 
  radio.begin();
  radio.setChannel(125);
  radio.setPALevel(RF24_PA_MIN);
  radio.enableDynamicPayloads();
  radio.setRetries(5, 15);
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
  radio.startListening();
  //inicializamos el puerto serie
  Serial.begin(115200);
  //inicializamos el sensor DHT
  dht.begin();

  if (tsl.begin()) 
  {
    Serial.println("Encontrado TSL2591");
  } 
  else 
  {
    Serial.println("TSL2591 no encontrado");
    while (1);
  }

  //Configuramos TSL2591
  configureTSL2591();
 
}
 
void loop()
{
  //Leemos sensores
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  flame = analogRead(IRFLAME_5V_PIN_AOUT);
  mq7 = analogRead(MQ7_5V_PIN_AOUT);
  mq2 = analogRead(MQ2_5V_PIN_AOUT);
  tsl_visible = tsl.getLuminosity(TSL2591_VISIBLE);
  
  //Imprimimos temperatura y humedad en el monitor serial
  Serial.print("Humedad: ");
  Serial.print(hum);
  Serial.print(" %, Temp: ");
  Serial.print(temp);
  Serial.print(" Celsius, Fogo:");
  Serial.print(flame);
  Serial.print(" MQ7: ");
  Serial.print(mq7);
  Serial.print(" MQ2: ");
  Serial.print(mq2);
  Serial.print(" TSL2591: ");
  Serial.println(tsl_visible);
  //cargamos los datos en la variable datos[]
  float datos [6];
  datos[0] = hum;
  datos[1] = temp;
  datos[2] = flame;
  datos[3] = mq7;
  datos[4] = mq2;
  datos[5] = tsl_visible;
  //enviamos los datos
  
  radio.stopListening();

  
  Serial.println(F("Enviando"));
  
  if (!radio.write(datos, sizeof(datos))) {
    Serial.println(F("No se ha podido enviar"));
  }

  radio.startListening();

  unsigned long started_waiting_at = millis();
  bool timeout = false;
  while (!radio.available() && !timeout) {
    if (millis() - started_waiting_at > 500 ) {
      timeout = true;
    }
  }
    
        
  if ( timeout ){                                             // Describe the results
      Serial.println(F("Failed, response timed out."));
  }else {
      // Grab the response, compare, and send to debugging spew
      uint8_t len = radio.getDynamicPayloadSize();
      
      // If a corrupt dynamic payload is received, it will be flushed
      if (!len) {
        return; 
      }
      
      radio.read(receive_payload, len);

      // Put a zero at the end for easy printing
      receive_payload[len] = 0;

      // Spew it
      Serial.print(F("Got response size="));
      Serial.print(len);
      Serial.print(F(" value="));
      Serial.println(receive_payload);
    }

  
  
  delay(5000);
}


void configureTSL2591(void)
{
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  //tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain
  
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  // tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)

  /* Display the gain and integration time for reference sake */  
  Serial.println(F("------------------------------------"));
  Serial.print  (F("Gain:         "));
  tsl2591Gain_t gain = tsl.getGain();
  switch(gain)
  {
    case TSL2591_GAIN_LOW:
      Serial.println(F("1x (Low)"));
      break;
    case TSL2591_GAIN_MED:
      Serial.println(F("25x (Medium)"));
      break;
    case TSL2591_GAIN_HIGH:
      Serial.println(F("428x (High)"));
      break;
    case TSL2591_GAIN_MAX:
      Serial.println(F("9876x (Max)"));
      break;
  }
  Serial.print  (F("Timing:       "));
  Serial.print((tsl.getTiming() + 1) * 100, DEC); 
  Serial.println(F(" ms"));
  Serial.println(F("------------------------------------"));
  Serial.println(F(""));
}
