#include <FS.h>                   
#include <ESP8266WiFi.h>          
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         
#include <PubSubClient.h>        
#include <ArduinoJson.h>         
#include <WiFiUDP.h>
#include <ArduinoOTA.h>
#include <MQTT.h>
#include <EEPROM.h>
#include <Arduino.h>
#include <EEPROM.h>

#define DEBUG1


struct StoreStruct {
    bool state1 ;     //state1
    
  } 
  
storage = {
  
  0, // off
  
};

int buttonState1;
boolean switchState1;
const int buttonPin1 = 4;     //  
const int outPin1 = 12;  
int lastButtonState1 = LOW; 




const int smartconfigled =  5;      // GPIO 5 the number of the LED pin   D1


// MQTT settings

WiFiClient espClient;

PubSubClient client(espClient);

long lastMsg = 0;

char msg[128];

long interval = 10000;     // interval at which to send mqtt messages (milliseconds)



//define your default values here, if there are different values in config.json, they are overwritten.

char mqtt_server[40] = "10.10.10.102";

char mqtt_port[6] = "1883";

char username[34] = "";

char password[34] = "";

const char* willTopic = "home/";

String node=String(ESP.getChipId());
const char* clientid=node.c_str();

const char* apName = clientid;
const char* apPass = clientid;



//flag for saving data

bool shouldSaveConfig = false;



//callback notifying us of the need to save config

void saveConfigCallback () {

  Serial.println("Should save config");

  shouldSaveConfig = true;

}



// Message received through MQTT

void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message arrived [");

  Serial.print(topic);

  Serial.print("] ");

  for (int i = 0; i < length; i++) {
    Serial.print("payload ");
    Serial.println((char)payload[i]);
    
  }

  



  // Switch on the relay if an 1 was received as first character on the home/ topic

  if (strcmp(topic, (String("home/relay")+clientid).c_str()) == 0){

    Serial.println("Topic recognized");

    if ((char)payload[0] == '1') {

    digitalWrite(outPin1, HIGH);   // Turn the Relay on
    
    switchState1 = !switchState1; 
   
      Serial.println(String("home/relay")+clientid+String("ON"));

      client.publish((String("home/relay")+clientid+String("/status")).c_str(), "ON");

    } 
    else if ((char)payload[0]=='0') {
    
    digitalWrite(outPin1, LOW);   // Turn the Relay on
     
    switchState1 = !switchState1; 
   
      Serial.println(String("home/relay")+clientid+String("OFF"));

      client.publish((String("home/relay")+clientid+String("/status")).c_str(), "OFF");
     
    }




  }

Serial.println();
Serial.print("\n");

}




void setup() {
  //ArduinoOTA.setPassword((const char *)"123");
  ArduinoOTA.begin();

  pinMode(outPin1, OUTPUT);     // Initialize the outPin as an output
  
  
  Serial.begin(115200);

  Serial.println();

  pinMode(buttonPin1, INPUT);
  digitalWrite(buttonPin1, HIGH);

  
  


  switchState1 = LOW;  
    


  

#ifndef DEBUG1
  pinMode(outPin1, OUTPUT); 
  digitalWrite(outPin1, switchState1);
#endif  



// initialize the LED pin as an output:
  pinMode(smartconfigled, OUTPUT);
  

  //clean FS, for testing

  //SPIFFS.format();



  //read configuration from FS json

  Serial.println("mounting FS...");



  if (SPIFFS.begin()) {

    Serial.println("mounted file system");

    if (SPIFFS.exists("/config.json")) {

      //file exists, reading and loading

      Serial.println("reading config file");

      File configFile = SPIFFS.open("/config.json", "r");

      if (configFile) {

        Serial.println("opened config file");

        size_t size = configFile.size();

        // Allocate a buffer to store contents of the file.

        std::unique_ptr<char[]> buf(new char[size]);



        configFile.readBytes(buf.get(), size);

        DynamicJsonBuffer jsonBuffer;

        JsonObject& json = jsonBuffer.parseObject(buf.get());

        json.printTo(Serial);

        if (json.success()) {

          Serial.println("\nparsed json");



          strcpy(mqtt_server, json["mqtt_server"]);

          strcpy(mqtt_port, json["mqtt_port"]);

          strcpy(username, json["username"]);

          strcpy(password, json["password"]);



        } else {

          Serial.println("failed to load json config");

        }

      }

    }

  } else {

    Serial.println("failed to mount FS");

  }

  
  int cnt = 0;
digitalWrite(smartconfigled, HIGH);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print (".");
    if (cnt++ >= 10) {
      WiFi.beginSmartConfig();
      while (1) {
        delay (1000);
        if (WiFi.smartConfigDone()) {
          Serial.println("Smart Config done");
          
          break;
        }
      }
    }
  }

  //end read



  //save the custom parameters to FS

  if (shouldSaveConfig) {

    Serial.println("saving config");

    DynamicJsonBuffer jsonBuffer;

    JsonObject& json = jsonBuffer.createObject();

    json["mqtt_server"] = mqtt_server;

    json["mqtt_port"] = mqtt_port;

    json["username"] = username;

    json["password"] = password;



    File configFile = SPIFFS.open("/config.json", "w");

    if (!configFile) {

      Serial.println("failed to open config file for writing");

    }



    json.printTo(Serial);

    json.printTo(configFile);

    configFile.close();

    //end save

  }



  Serial.println("local ip");

  Serial.println(WiFi.localIP());

  Serial.println(clientid);

  // mqtt

  client.setServer(mqtt_server, atoi(mqtt_port)); // parseInt to the port

  client.setCallback(callback);



}





void reconnect() {

  // Loop until we're reconnected to the MQTT server

  while (!client.connected()) {

    Serial.print("Attempting MQTT connection...");

    // Attempt to connect

    if (client.connect(username, username, password, willTopic, 0, 1, (String("I'm dead: ")+username).c_str())) {     // username as client ID

      Serial.println("connected");
      
      digitalWrite(smartconfigled, LOW);
      
      // Once connected, publish an announcement... (if not authorized to publish the connection is closed)

      client.publish("all", (String("home/relay")+clientid).c_str());

      // ... and resubscribe

      client.subscribe((String("home/relay")+clientid).c_str());


      delay(5000);

      

    } else {

      Serial.print("failed, rc=");

      Serial.print(client.state());

      Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying

      delay(5000);

    }

  }

}





void loop() {

int reading1 = digitalRead(buttonPin1);

  if (reading1 != lastButtonState1) {
    if (reading1 == LOW)
    {
      switchState1 = !switchState1;            
#ifdef DEBUG1
      Serial.println("button1 pressed");
      digitalWrite(outPin1, switchState1);   // Turn the Relay on
      Serial.println(String("home/relay")+clientid+String(" : ")+String(switchState1));
      client.publish((String("home/relay")+clientid+String("/status")).c_str(),String(switchState1).c_str());
      
#endif
    }

    lastButtonState1 = reading1;
  }
  

#ifndef DEBUG1 
  digitalWrite(outPin1, switchState1); 
#endif
  if (switchState1 != storage.state1)
  {
    storage.state1 = switchState1;

  }


 



  if (!client.connected()) { // MQTT

    reconnect();

  }

  client.loop();



  unsigned long now = millis();

 

  if(now - lastMsg > interval) {

    

    lastMsg = now;

     Serial.println(String("home/relay")+clientid+String("/status"));
     Serial.println("Pin1: ");
     Serial.println(storage.state1);
     

  }

 
 ArduinoOTA.handle(); 

}



