#include <ArduinoJson.h>


#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN D5

int tab_stations[5] = {2013, 2014, 2025, 2016, 2017};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(5, PIN, NEO_GRB + NEO_KHZ800);

//const char* host = "maker.ifttt.com";
const char* host = "velov-map.herokuapp.com";

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialisation des LEDs
  strip.begin();
  strip.setPixelColor(0, 100, 100, 0);
  strip.setPixelColor(1, 100, 100, 0);
  strip.setPixelColor(2, 100, 100, 0);
  strip.setPixelColor(3, 100, 100, 0);
  strip.setPixelColor(4, 100, 100, 0);
  strip.show(); // Initialize all pixels to 'off'
  
  /*//set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);*/

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("EconoSwitch")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  /*ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);*/

  //hit_api("10002");

  /*int i = 0;
  for(i = 0; i < 2; i++){
    Serial.println(tab_stations[i]);
    hit_api(tab_stations[i], i);
  }*/
}


void loop() {
 int i = 0;
  for(i = 0; i < 2; i++){
    Serial.println(tab_stations[i]);
    hit_api(tab_stations[i], i);
  }
}

void hit_api(int station, int id_led) {
  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  char url[100] = "/station/";
  char str[12];
  sprintf(str, "%d", station);
  Serial.println(str);
  strcat(url, str);
  
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');

    //Serial.println(line);
    //Serial.println(line[1]);
    
    if(line[1] == '{'){ //Si on a le JSON
      Serial.print(line);

      StaticJsonBuffer<200> jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(line);
      if (!root.success()) {
        Serial.println("parseObject() failed");
      }
      else {
        int velov = root["velov"];
        int places = root["places"];

        Serial.println(velov);
        Serial.println(places);

        setStation(id_led, calcPourcentage(velov, places));
      }
    }
  }
  
  Serial.println();
  Serial.println("closing connection");
}

int calcPourcentage(int nbVelov, int nbPlaces) 
{
  return (nbVelov * 100) / (nbVelov + nbPlaces);
}

void setStation(int numStation, int p_occupation)
{
  int p = (p_occupation * 255) / 100;

  Serial.println(p);
  
  strip.setPixelColor(numStation, 255-p, p, 0);
  strip.show();
}
