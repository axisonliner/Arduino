#include <ESP8266WiFi.h>
#include <Credentials.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#define firmware "iotupdater"
#define firmware_version firmware"_001"
#define NUMPIXELS      12
#include <Adafruit_NeoPixel.h>
#define PIXEL_PIN 12
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
//---------- MQTT -------
const char* mqttServer = "192.168.1.33";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

#define BUFFER_SIZE 100
char* lighter = "home/lights/bedroom/lighter";
//char* brightnessTop = "home/lights/bedroom/lighter/brightness";
char* lighterTopic = "home/lights/bedroom/lighter/#";
//char* lightstat = "home/lights/bedroom/lighter/lightstat";
WiFiClient espClient;
PubSubClient client(espClient);

int hue = 0;
float brightness = 1.0;
float saturation = 0.0;
int lasthue = 0;
float lastbrightness = 0.0;
float lastsaturation = 0.0;
float tempBrightness = 1.0;

String daystatus = "DAY";
String lastTemp;
String lastHumi;

//int pubState = 0;
int humiState = 0;
int currentStatus = 0;
int green = 0; 
//int laststate = 0;
int j = 0;

float h = 0.0;
float t = 0.0; 

const int LIGHT_SWICH_PIN = 14;

String Temp = "0";
String Humi = "0";

DHT dht(13, DHT22, 20);

void reconnect() { ////////////// Reconnecting to Wi-Fi ////////////////////
    Serial.print("Reconnecting to Wi-Fi...");
    WiFi.begin(mySSID, myPASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (j == 12) {
            colorWipe(strip.Color(0, 0, 0), 5);
            j = 0;
        }
        strip.setPixelColor(j, strip.Color(50,50,255)); 
        strip.show();  
        Serial.print(".");
        j=j+1;
    } 
    j = 0;      
    Serial.print("connected to ");
    Serial.println(mySSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    colorWipe(strip.Color(0, 0, 0), 5);
}

void colorWipe(uint32_t c, uint8_t wait) {
    for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
        strip.show();
        delay(wait);
    }
}
/*
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

void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void analogSens() {
// read the input on analog pin:
int sensLight = analogRead(A0);
  if (sensLight >= 150) {
        daystatus = "DAY";
  } else {
          daystatus = "NIGHT";
          //rainbowCycle(10);
    }  
    delay(100); // 10 delay in between reads for stability
}
*/
uint32_t HSVColor(float h, float s, float v) {

    h = constrain(h, 0, 360);
    s = constrain(s, 0, 1);
    v = constrain(v, 0, 1);

    int i, b, p, q, t;
    float f;

    h /= 60.0;  // sector 0 to 5
    i = floor( h );
    f = h - i;  // factorial part of h

    b = v * 255;
    p = v * ( 1 - s ) * 255;
    q = v * ( 1 - s * f ) * 255;
    t = v * ( 1 - s * ( 1 - f ) ) * 255;

    switch( i ) {
        case 0:
          return strip.Color(b, t, p);
        case 1:
          return strip.Color(q, b, p);
        case 2:
          return strip.Color(p, b, t);
        case 3:
          return strip.Color(p, q, b);
        case 4:
          return strip.Color(t, p, b);
        default:
          return strip.Color(b, p, q);
    }
}

void currentValues(){
  Serial.println("");
  Serial.println("Current State");
  Serial.print("Hue (0-360):");
  Serial.println(hue);
  Serial.print("Saturation (0-100 in, 0-1):");
  Serial.println(saturation*100);
  Serial.print("Brightness (0-100):");
  Serial.println(brightness*100);
  Serial.println("");
}

void callback(char* topic, byte* payload, unsigned int length) {
    uint16_t i, j;
    String newMessage = "";
    Serial.print("Message arrived: [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        newMessage.concat((char)payload[i]);  
    }
    Serial.println(newMessage);
    String myTopic = String(topic);
    // ----------------------------------------
    if(myTopic == lighter) {
        if (newMessage == "1") {
            if (tempBrightness != 0.0) {
                brightness = tempBrightness;
            } 
            //brightness = 1.0;
            while (lastbrightness <= brightness) {
                for(i=0; i<strip.numPixels(); i++) {
                lastbrightness += 0.002;
                strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
            }
            lastbrightness = brightness;                   
        }
        else {
            brightness = 0.0;
            tempBrightness = lastbrightness;    
            while (lastbrightness >= brightness) {
                lastbrightness -= 0.002;                
                for(i=0; i<strip.numPixels(); i++) {
                strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
            }
            lastbrightness = brightness;     
        }
    }
    // ----------------------------------------
    if (myTopic == "home/lights/bedroom/lighter/brightness") { // Brightness up to 100 
        brightness = newMessage.toFloat()/100;
        if (brightness > 0.99){
          digitalWrite(LIGHT_SWICH_PIN, HIGH);
        } else {
          digitalWrite(LIGHT_SWICH_PIN, LOW);
        }
        if (brightness >= lastbrightness) {
            while (lastbrightness <= brightness) {
                for(i=0; i<strip.numPixels(); i++) {
                strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();      
                lastbrightness += 0.002;  
            }
            lastbrightness = brightness;
        } 
        if (brightness <= lastbrightness) {
            while (lastbrightness >= brightness) {
                for(i=0; i<strip.numPixels(); i++) {
                strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
                lastbrightness -= 0.002;
            }
            lastbrightness = brightness;
        }           
      }
    // ------------------ Hue value 0-360 -----------------------
    if (myTopic == "home/lights/bedroom/lighter/hue") { 
        hue = newMessage.toInt();
        if (hue >= lasthue) {
            while (lasthue <= hue) {
                for(i=0; i<strip.numPixels(); i++) {
                strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
                lasthue += 1;
                delay(30); 
            }
            lasthue = hue;
        }
        if (hue <= lasthue) {
            while (lasthue >= hue) {
                for(i=0; i<strip.numPixels(); i++) {
                strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
                lasthue -= 1;
                delay(30);
            }
            lasthue = hue;      
        }
    }        
    //-------------------- Saturation value at 0-100 ------------------------ 
    if(myTopic == "home/lights/bedroom/lighter/saturation")  { 
        saturation = newMessage.toFloat();
        if (saturation >= lastsaturation) {
            while (lastsaturation <= saturation) {
                for(i=0; i<strip.numPixels(); i++) {
                strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
                lastsaturation += 0.1;
                // delay(30);       
            }
            lastsaturation = saturation;
        }
        if (saturation <= lastsaturation) {
            while (lastsaturation >= saturation) {
                for(i=0; i<strip.numPixels(); i++) {
                strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
                lastsaturation -= 0.1;
                // delay(30);        
            }
            lastsaturation = saturation;
        }
    }       
} 

void tempH(){///////////////  TEMP & HUMI //////////            
    if (humiState == 0) {  
        delay(10); 
        h = dht.readHumidity();
        t = dht.readTemperature();
        if (isnan(h) || isnan(t)) {
            return;
        }     
        char Temp[8];
        dtostrf(t, 5, 1, Temp);
        char Humi[8];
        dtostrf(h, 5, 1, Humi); 
        if (lastTemp != Temp) {                                                                          
            client.publish("home/lights/bedroom/lighter/temp", Temp); 
            lastTemp = Temp; 
        } 
        if (lastHumi != Humi) {
            lastHumi = Humi;                                        
            client.publish("home/lights/bedroom/lighter/humi", Humi);
        }                 
    }
}

void setup() {
    pinMode(LIGHT_SWICH_PIN, OUTPUT);   // Initialize the LED_BUILTIN pin as an output
    digitalWrite(LIGHT_SWICH_PIN, LOW);  // Turn the LED on (Note that LOW is the voltage level   
    //************* Conneting to Wi-Fi ********************************************* 
    Serial.begin(115200);
    Serial.println("The table lighter in bedroom v.54 (26.09.2018)");
    Serial.println("");
    Serial.print("Start conneting to Wi-Fi");
    // End of trinket special code
    strip.begin(); // This initializes the NeoPixel library.
    strip.show(); // Initialize all pixels to 'off'   
    WiFi.begin(mySSID, myPASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (j == 12) {
            colorWipe(strip.Color(0, 0, 0), 5);
            j = 0;
        }
        strip.setPixelColor(j, strip.Color(50,50,255)); 
        strip.show();  
        Serial.print(".");
        j += 1;
    } 
    j = 0;
    Serial.print("connected to ");
    Serial.println(mySSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    colorWipe(strip.Color(0, 0, 0), 5);
    // MQTT callback
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    //************* Reading MAC adress ********************************************* 
    byte mac[6];
    WiFi.macAddress(mac);
    Serial.print("MAC: ");
    for (int i = 0; i < 5; i++) {
        Serial.print(mac[i], HEX);
        Serial.print(":");
    }
    Serial.println(mac[5], HEX);
     //*** Cheking for update from web using  http://192.168.1.3/iotappstorev10.php ***
    Serial.println("");
    Serial.println("Cheking for new update..."); 
    t_httpUpdate_return ret = ESPhttpUpdate.update("http://192.168.1.3/iotappstorev10.php", firmware_version);        
        switch(ret) {
            case HTTP_UPDATE_FAILED:
                Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                Serial.println("");
                break;
            case HTTP_UPDATE_NO_UPDATES:
                Serial.println("HTTP_UPDATE_NO_UPDATES");
                Serial.println("");
                break;
            case HTTP_UPDATE_OK:
                Serial.println("HTTP_UPDATE_OK");
                Serial.println("");
                break;
        }         
}
void loop() {   
    if (WiFi.status() == 6) {
        reconnect();
    }
    if (WiFi.status() == WL_CONNECTED) {
        if (!client.connected()) {
            Serial.print("Connecting to MQTT server");
            //Serial.print(mqttServer);
            Serial.println("...");
            if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
                Serial.println("Connected!!!");
                client.setCallback(callback);
                client.subscribe(lighterTopic);
            } else {                
                Serial.print("");
                Serial.println("Could not connect to MQTT server ");
                Serial.print("failed state ");
                Serial.println(client.state());                   
                while (green == 255) {
                    green += 1;                    
                    for(int i=0; i<strip.numPixels(); i++) {
                        strip.setPixelColor(i, strip.Color(50,green,50));
                    }
                    strip.show();
                    delay(30);                
                }
                green = 255;
                while (green == 0) {
                    green -= 1;
                    for(int i=0; i<strip.numPixels(); i++) {
                        strip.setPixelColor(i, strip.Color(0,green,0));
                    }
                    strip.show();           
                } 
                green = 0;                
                delay(2000); 
            }
        }    
        if (client.connected()){
          tempH();
          //analogSens();
          delay(100);
          client.loop();
        }         
    }
}
