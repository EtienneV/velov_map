#include <ArduinoJson.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiClientSecure.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN D5

#define NB_STATIONS 8

// TODOs :
// Optimise API call : 1 call for all stations
// Personnal server 
// Blinking on interruption to keep blinking during download. No : call blink function during downloading loop

int nb_stations = NB_STATIONS;
int tab_stations[NB_STATIONS] = {10031, 10001, 10103, 10002, 10101, 10056, 10102, 10084}; //{10031, 10001, 10103, 10002, 10101, 10056, 10102, 10084}; // Map : La Doua
int tab_velov[NB_STATIONS] = {0, 0, 0, 0, 0, 0, 0, 0};
int tab_places[NB_STATIONS] = {0, 0, 0, 0, 0, 0, 0, 0};
int tab_etat_led[NB_STATIONS] = {0, 0, 0, 0, 0, 0, 0, 0};

enum JsonParsingState {SEARCH_STATION, READ_STATIONS_ID, SEARCH_FIELD, READ_FIELD, END};
enum FieldType {BIKES, DOCKS};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NB_STATIONS, PIN, NEO_GRB + NEO_KHZ800);

//const char* host = "maker.ifttt.com";
//const char* host = "velov-map.herokuapp.com";
const char* host = "api.cyclocity.fr";

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

  strip.begin(); // Initialisation des LEDs
  strip.clear(); // Set all pixel colors to 'off'

  int i = 0;
  for(i = 0; i < nb_stations; i++){
    strip.setPixelColor(i, 25, 25, 50);
  }
  
  strip.show(); // Apply colors to the pixels

  //theaterChaseRainbow(50);
  
  /*//set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);*/


  //WiFi.mode(WIFI_OFF);
  //delay(1000);
  //WiFi.forceSleepBegin();
  //delay(1000);
  //WiFi.forceSleepWake();
  //delay(1000);
  //WiFi.mode(WIFI_STA);
  //WiFi.disconnect(true);
  //delay(1000);






  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  wifiManager.setCleanConnect(true);

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Carte Velov")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println(F("Connexion au Wifi reussie"));

  //rainbow(20);

  maj_stations();
}

long seconds = 0;
unsigned long time_blink = millis();
int sequence = 0;

void loop() {
  delay(500); // Pause d'une seconde

  if(seconds >= 120) // Si ça fait une minute
  {
    Serial.println("UPDATE");
    maj_stations();
    seconds = 0;
  }

  blink_loop_fn();

  seconds++;
}


void blink_loop_fn() {
  
  if((millis() - time_blink) >= 500) 
  {
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

    sequence++;

    if(sequence == 4) sequence = 0;
    
    time_blink = millis();
  }
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
  Serial.println("Update stations");

  hit_api();

  int i = 0;
  for(i = 0; i < nb_stations; i++){
      setStation(i, calcPourcentage(tab_velov[i], tab_places[i]));
      tab_etat_led[i] = 1;
  }
}








void hit_api() {
  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  client.setInsecure();   // ! TLS sans vérification
  const int httpPort = 443;
  if (!client.connect(host, httpPort)) {
    Serial.println("HTTPS connection failed");
    return;
  }
  
  // We now create a URI for the request
  const char* url = "/contracts/lyon/gbfs/station_status.json";  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print("GET ");
  client.print(url);
  client.print(" HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\nConnection: close\r\n\r\n");


  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
    blink_loop_fn();
    yield();
  }

  // 1️⃣ Skip HTTP headers
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    //Serial.println(line);

    if (line == "\r") break;
    blink_loop_fn();
    yield();
  }


  
  JsonParsingState state = SEARCH_STATION;
  FieldType current_field = BIKES;
  String buffer = "";
  //int i = -1;
  String number_string_buffer = "";
  int current_station_id = -1;
  int current_station_index = 0;
  bool found_bikes = false;
  bool found_docks = false;
  bool stations_found_mask[NB_STATIONS];
  bool continue_reading = true;

  for(int i = 0; i < NB_STATIONS; i++) {
    stations_found_mask[i] = false;
  }


  // 3️⃣ Lecture streamée
  while ((client.connected() || client.available()) && continue_reading) {
    if (client.available()) {
      char c = client.read();
      buffer += c;

      //Serial.println(buffer);

      // Limite buffer pour éviter explosion RAM
      if (buffer.length() > 200) {
        buffer.remove(0, buffer.length() - 200);
      }

      //Serial.println(buffer);



      switch (state) {
        case SEARCH_STATION: {
          //Serial.println("State SEARCH_STATION");

          bool continue_searching_stations = false;

          for(int i = 0; i < NB_STATIONS; i++) {
            if (stations_found_mask[i] == false) {
              continue_searching_stations = true;
            }
          }

          if (continue_searching_stations) {
            if (buffer.indexOf("\"station_id\":\"") != -1) {
              //Serial.println("Found field -> station_id:");

              state = READ_STATIONS_ID;

              number_string_buffer = "";
              current_station_id = -1;
              buffer = "";
            }
          }
          else {
            Serial.println("All stations found !");

            state = END;
            continue_reading = false;
            buffer = "";
          }

          break;
        }
        
        case READ_STATIONS_ID: {
          //Serial.println("State READ_STATIONS_ID");

          if (isDigit(c)) {
            //Serial.println("Digit");
            number_string_buffer += c;
          }
          else {
            //Serial.println("NOT a digit");

            current_station_id = number_string_buffer.toInt();

            //Serial.print("Station id buffer : ");
            //Serial.println(number_string_buffer);
            //Serial.print("Current station ID : ");
            //Serial.println(current_station_id);

            bool found_station = false;

            for (int i = 0; i < NB_STATIONS; i++) {
              if (tab_stations[i] == current_station_id) {
                Serial.println("STATION IN LIST");
                
                current_station_index = i;

                found_station = true;
                break;
              }
            }

            if (found_station) {
              state = SEARCH_FIELD;

              found_bikes = false;
              found_docks = false;

              number_string_buffer = "";
              buffer = "";
            }
            else {
              //Serial.println("End of FOR");

              state = SEARCH_STATION;
              buffer = "";
            }
          }
          break;
        }

        case SEARCH_FIELD: {
          //Serial.println("State SEARCH_FIELD");
          //Serial.print("Station index : ");
          //Serial.println(current_station_index);
          
          if (buffer.indexOf("\"num_bikes_available\":") != -1) {
            //Serial.println("Found field -> num_bikes_available:");

            state = READ_FIELD;
            current_field = BIKES;

            number_string_buffer = "";
            buffer = "";
          }
          else if (buffer.indexOf("\"num_docks_available\":") != -1) {
            //Serial.println("Found field -> num_docks_available:");

            state = READ_FIELD;
            current_field = DOCKS;

            number_string_buffer = "";
            buffer = "";
          }

          break;
        }

        case READ_FIELD: {
          //Serial.println("State READ_FIELD");
          //Serial.print("Station index : ");
          //Serial.println(current_station_index);
          //Serial.print("Current field : ");
          //Serial.println(current_field);



          if (isDigit(c)) {
            //Serial.println("Digit");
            number_string_buffer += c;
          }
          else {
            //Serial.println("NOT a digit");

            int field_value = number_string_buffer.toInt();

            Serial.print("Field value : ");
            Serial.println(field_value);

            if (current_field == BIKES) {
              Serial.print("Update bike with value : ");
              Serial.println(field_value); 

              tab_velov[current_station_index] = field_value;
              found_bikes = true;
            }
            else if (current_field == DOCKS) {
              Serial.print("Update dock with value : ");
              Serial.println(field_value); 
              
              tab_places[current_station_index] = field_value;
              found_docks = true;
            }

            if (found_bikes && found_docks) {
              Serial.println("Bike AND docks found !");

              stations_found_mask[current_station_index] = true;

              state = SEARCH_STATION;
              buffer = "";
            }
            else {
              Serial.println("Search other field");

              state = SEARCH_FIELD;
              buffer = "";
            }
          }



          break;
        }
        
        default:
          Serial.println("DEFAULT STATE");
        
      }
    }

    blink_loop_fn();

    yield();
  }

  client.stop();



  for (int i = 0; i < NB_STATIONS; i++) {
    int station = tab_stations[i]; 
    int bikes = tab_velov[i];
    int places = tab_places[i];

    Serial.print("Station : ");
    Serial.print(station);
    Serial.print(" --> ");
    Serial.print(bikes);
    Serial.print(" / ");
    Serial.println(places);
  }


}




















int calcPourcentage(int nbVelov, int nbPlaces) 
{
  if ((nbVelov + nbPlaces) == 0) {
    return 0;
  }

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
