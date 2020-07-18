  
#include <Wire.h>                 // Must include Wire library for I2C
#include "SparkFun_MMA8452Q.h"    // Click here to get the library: http://librarymanager/All#SparkFun_MMA8452Q
// Librairie pour ESP8266 WIFI
//#include <ESP8266WiFi.h>//
//#include <ESP8266WiFiMulti.h> 
// Librairie pour la gestion MQTT
#include <WiFi.h>
#include <PubSubClient.h> 
#define SDA_PIN 23
#define SCL_PIN 22
#define TRIGGER_PIN  4  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     5  // Arduino pin tied to echo pin on the ultrasonic sensor.
 
long duration, distance; // Duration used to calculate distance
MMA8452Q accel;                   // create instance of the MMA8452 class

// WIFI
const char* ssid = "AndroidAP";
const char* password = "sxle2088";
// MQTT
const int mqtt_port = 1883; // Port utilisé par le Broker 
const char* mqtt_server = "192.168.43.105"; // Adresse IP du Broker MQTT
// Topics MQTT 
const char* topic_data = "drone_iot/devices/data";

WiFiClient espClient;
PubSubClient client(espClient);
 
void setup()
{
Serial.begin (9600);
Serial.println("Let's go !");
accel.init();
Wire.begin(SDA_PIN, SCL_PIN);
accel.read();
if (accel.begin() == false) {
  Serial.println("Not Connected. Please check connections and read the hookup guide.");
  while (1);
}
pinMode(TRIGGER_PIN, OUTPUT);
pinMode(ECHO_PIN, INPUT);

setup_wifi();
setup_mqtt();
}
 
void loop() {
  // Inclinaison :
  // Check MQTT connection
  reconnect();
  client.loop(); 
  byte pl = accel.readPL();
    switch (pl)
    {
    case PORTRAIT_U:
        Serial.print("Portrait Up :");
        break;
    case PORTRAIT_D:
        Serial.print("Portrait Down :");
        break;
    case LANDSCAPE_R:
        Serial.print("Landscape Right :");
        break;
    case LANDSCAPE_L:
        Serial.print("Landscape Left :");
        break;
    case LOCKOUT:
        Serial.print("Flat :");
        break;
    }
  
  // TRigger l'echo des US
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  //Calculate the distance (in cm) based on the speed of sound.
  distance = duration/58.2;
  Serial.print("Distance avant un obstacle (cm): ");
  Serial.println(distance);
  moyenne(distance);
}


void moyenne(long distance) {

  float moyenne = 0; 
  float tab[5]; // un trajet de 30 secondes car 6 * 5 secondes 
  for (int x = 0; x < 5 ; x++) 
    {
      float accelX = accel.getCalculatedX() * 9.81; // accélération en m/s²
      float velocityX = accelX * 1; // 1 si c'est toute les 1 secondes pour avoir la vitesse en m/s
      if (accel.available()) {      // Wait for new data from accelerometer
        // Acceleration of x, y, and z directions in g units
        //delay(1000);
        Serial.print("Acceleration in X-Axis : ");
        Serial.print(accel.getCalculatedX(), 3);
        Serial.println("g");
        Serial.print("vitesse :");
        Serial.print(velocityX);
        Serial.println("m/s");
        Serial.print("\t");
        //delay(1000);
        Serial.print("Acceleration in Y-Axis : ");
        Serial.print(accel.getCalculatedY(), 3);
        Serial.print("g");
        Serial.print("\t");
        //delay(1000);
        Serial.print("Acceleration in Z-Axis : ");
        Serial.print(accel.getCalculatedZ(), 3);
        Serial.println("g");
        delay(1000);// 1 itération de secondes
      }
        tab[x] += velocityX; 
        
        if ((x+1)%5 == 0) { 
          moyenne = (tab[x] + tab[x-1] + tab[x-2] + tab[x-3] + tab[x-4])/5; // calcul de la moyenne toute les 5 valeurs de vitesse (toute les 5 secondes) 
          // publish sur MQTT de moyenne
          Serial.print("Moyenne :");
          Serial.print(moyenne);  
          Serial.println("m/s");
        }
   } 
   mqtt_publish(topic_data, moyenne, distance);
} 

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_mqtt() {
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Déclaration de la fonction de souscription
  reconnect();
}

void reconnect(){
  while (!client.connected()) {
    Serial.println("Connection au serveur MQTT ...");
    if (client.connect("ESP32Client")) {
      Serial.println("MQTT CONNECTE !!!");
    }
    else {
      Serial.print("echec, code erreur= ");
      Serial.println(client.state());
      Serial.println("nouvel essai dans 2s");
    delay(2000);
    }
  }
}

void mqtt_publish(String topic, float t, long distance){
  char top[topic.length()+1];
  topic.toCharArray(top,topic.length()+1);
  char json_value[200];
  String t_str = "{speed:" + String(t) + ", distance:" + String(distance) + "}";
  t_str.toCharArray(json_value, t_str.length() + 1);
  client.publish(top, json_value);
}


void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}
