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

int nb_stations = 5;
int tab_stations[5] = {2013, 2014, 2025, 2016, 2017};
int tab_velov[5] = {0, 0, 0, 0, 0};
int tab_places[5] = {0, 0, 0, 0, 0};
int tab_etat_led[5] = {0, 0, 0, 0, 0};

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
  /*strip.setPixelColor(0, 0, 50, 50);
  strip.setPixelColor(1, 0, 50, 50);
  strip.setPixelColor(2, 0, 50, 50);
  strip.setPixelColor(3, 0, 50, 50);
  strip.setPixelColor(4, 0, 50, 50);*/

  int i = 0;
  for(i = 0; i < nb_stations; i++){
    strip.setPixelColor(i, 0, 50, 50);
  }
  
  
  strip.show(); // Initialize all pixels to 'off'

  //theaterChaseRainbow(50);
  
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

  //rainbow(20);
  maj_stations();
}

long seconds = 0;
int sequence = 0;

void loop() {
  delay(500); // Pause d'une seconde

  if(seconds >= 120) // Si ça fait une minute
  {
    maj_stations();
    seconds = 0;
  }

  int i = 0;
  for(i = 0; i < nb_stations; i++){
    if(tab_velov[i] == 0){
      changer_etat_led(i);
    }
    else if(tab_places[i] == 0){
      changer_etat_led(i);
    }
      
    if((sequence == 0) || (sequence == 2)) { // Si on est sur du changement toutes les secondes
      if((tab_velov[i] < 3) && (tab_velov[i] != 0)){
        changer_etat_led(i);
      }
      else if((tab_places[i] < 3) && (tab_places[i] != 0)){
        changer_etat_led(i);
      }
    }
  }

  seconds++;
  sequence++;

  if(sequence == 4) sequence = 0;
}

void changer_etat_led(int i) {
  int p = calcPourcentage(tab_velov[i], tab_places[i]);
  
      if(tab_etat_led[i] == 0) {
        setStation(i, p);
        tab_etat_led[i] = 1;
      }
      else {
        strip.setPixelColor(i, (100-p)/2, p/2, 0);
        strip.show();
        tab_etat_led[i] = 0;
      }
}

void maj_stations(){  
  int i = 0;
  for(i = 0; i < nb_stations; i++){
      //Serial.println(tab_stations[i]);
      hit_api(tab_stations[i], i);
      setStation(i, calcPourcentage(tab_velov[i], tab_places[i]));
      tab_etat_led[i] = 1;
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
    
    if(line[1] == '{'){ //Si on a le JSON
      Serial.print(line);

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, line);

      if (error) {
        Serial.println("parseObject() failed");
      }
      else {
        int velov = doc["velov"];
        int places = doc["places"];

        tab_velov[id_led] = velov;
        tab_places[id_led] = places;
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

  //Serial.println(p);
  
  strip.setPixelColor(numStation, 100-p_occupation, p_occupation, 0);
  strip.show();
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
