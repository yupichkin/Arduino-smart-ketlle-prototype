
#include <SoftwareSerial.h>
#include <espduino.h>
#include <Ultrasonic.h>

SoftwareSerial espPort(2, 3); // RX, TX

ESP esp(&espPort, &Serial, -1);
const int ledpin = 13;
const int relay = 5;
const int termwater = A0;
int terma, set, status1, statusauto;

Ultrasonic ultrasonic(8,7);

void wifiCb(uint8_t status)
{
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println(F("WIFI: Connected!!!") );
  esp.mqttConnect("10.42.0.1", 1883);
}

void mqttConnected(uint32_t* args)
{
  Serial.println(F("MQTT:Connected"));
  esp.subscribe("sesc/tea/temp/set");
  esp.subscribe("sesc/tea/power/set");
  esp.subscribe("sesc/tea/auto");

}

void mqttData(String topic, String data)
{
  Serial.print(F("Received, topic:"));
  Serial.print(topic);
  Serial.print(F(", data:"));
  Serial.println(data);
  if (topic == "sesc/tea/temp/set") {
    Serial.println(F("SETTING TEMP"));
    set = data.toInt();
  } else 
  if (topic == "sesc/tea/power/set") {
    if (data == "OFF") {
      digitalWrite(relay, HIGH);
      status1 = 0;
    }
    else {
      if (data == "ON") {
        digitalWrite(relay, LOW);
        status1 = 1;
      }
    }
    esp.publish("sesc/tea/power/get", status1==1 ? "ON" : "OFF", 1, 0);
    Serial.println(F("SENDING RELAY"));
  }
  else {
    if (topic == "sesc/tea/auto") {
      if (data == "OFF") {
        statusauto = 0;
      }
      else {
        if (data == "ON") {
          statusauto = 1;
        }

      }
    }
  }

}

double Getterm(int RawADC) {
  double temp;
  temp = log(((10240000 / RawADC) - 10000));
  temp = 1 / (0.001129148 + (0.000234125 * temp) + (0.0000000876741 * temp * temp * temp));
  temp = temp - 273.15;
  return temp;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  Serial.begin(115200);
  espPort.begin(9600);

  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);

  /* setup event */
  esp.wifiCb = &wifiCb;
  esp.mqttConnected = &mqttConnected;
  esp.mqttData = &mqttData;

  /* Init data */
  esp.initMqttClient("teapot", "", "", 30);

  /* wifi connect */
  esp.wifiConnect("SmartHouse", "");
  
  

}


int lastT, term , term1, lastP, S,lastS;


void loop() {
  esp.process();


  int T = Getterm( analogRead(termwater));

  if (T != lastT) {

    esp.publish("sesc/tea/temp/get", String(T), 1, 0);
    esp.process();
    lastT = T;
    Serial.println(T);
    delay(500);
  }
  int S = ultrasonic.Ranging(CM);

  if (S != lastS) {

    esp.publish("sesc/tea/waterlvl", String(S*10), 1, 0);
    esp.process();
    lastS = S;
    Serial.println(S);
    delay(500);
  }

  if (statusauto == 1 ) {
    if (set > T) {
      digitalWrite(relay, LOW);
    }
    else {
      digitalWrite(relay, HIGH);
    }
  } else {

    if (status1 == 1) {
      digitalWrite(relay, LOW);
    }
    else {
      digitalWrite(relay, HIGH);
    }
  }
  
  int p = digitalRead(relay);
  if ( (p != lastP) && (p == 0) ) {
    Serial.println(F("ON"));
    esp.publish("sesc/tea/power/get", "ON", 1, 0);
    esp.process();
    lastP = p;
  }
  if ( (p != lastP) && (p == 1) ) {
    Serial.println(F("OFF"));
    esp.publish("sesc/tea/power/get", "OFF", 1, 0);
    esp.process();
    lastP = p;
  }
  
}

/*
  esp.publish("sesc/tea/power/get", "ON", 1,0);
  esp.process();
  delay(2000);
  esp.publish("sesc/tea/power/get", "OFF", 1,0);
    esp.process();
  delay(2000);
*/
