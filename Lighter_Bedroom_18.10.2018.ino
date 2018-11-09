#include <ESP8266WiFi.h>
#include <Credentials.h>
#include <PubSubClient.h>
// #include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#define firmware "iotupdater"
#define firmware_version firmware"_001"
#define NUMPIXELS      12
#define PIXEL_PIN 12
#define BUFFER_SIZE 100
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
//---------- MQTT -------
const char* mqttServer = "192.168.1.33";
// const int httpsPort = 443;
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
const int LIGHT_SWICH_PIN = 14;

char* lighter = "home/lights/bedroom/lighter";
char* lighterTopic = "home/lights/bedroom/lighter/#";

WiFiClient espClient;
PubSubClient client(espClient);

float brightness = 1.0;
float saturation = 0.0;
float lastbrightness = 0.0;
float lastsaturation = 0.0;
float tempBrightness = 1.0;
float firstBrightness = 0.0;
float h = 0.0;
float t = 0.0; 

String daystatus = "DAY";
String light = "OFF";
String lamp = "OFF";
String lastTemp;
String lastHumi;
String Temp = "0";
String Humi = "0";
// String bigLamp = "OFF";

int lastMessage = 0;
int lasthue = 0;
int hue = 0;
int humiState = 0;
int currentStatus = 0;
int green = 0; 
int j = 0;

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
void reconnectMQTT() {
    j = 0; 
    while (!client.connected()) {
        Serial.print("Connecting to MQTT server...");
        delay(500);
        if (j == 12) {
            colorWipe(strip.Color(0, 0, 0), 5);
            j = 0;
        }
        strip.setPixelColor(j, strip.Color(50,255,50)); 
        strip.show();  
        Serial.print(".");
        j += 1;               
        if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
            Serial.println("Connected!!!");
            client.setCallback(callback);
            client.subscribe(lighterTopic);
            colorWipe(strip.Color(0, 0, 0), 5);
            // rainbowCycle(3);                 
        } else {
            delay(1000);
        }
    }    
    j = 0;    
}

void colorWipe(uint32_t c, uint8_t wait) {
    for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
        strip.show();
        delay(wait);
    }
}

// uint32_t Wheel(byte WheelPos) {
//   WheelPos = 255 - WheelPos;
//   if(WheelPos < 85) {
//     return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
//   }
//   if(WheelPos < 170) {
//     WheelPos -= 85;
//     return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
//   }
//   WheelPos -= 170;
//   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
// }

// void rainbowCycle(uint8_t wait) {
//   uint16_t i, j;
//   for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
//     for(i=0; i< strip.numPixels(); i++) {
//       strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
//     }
//     strip.show();
//     delay(wait);
//   }
// }
/*
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
    if (myTopic == lighter && lastMessage == 0 && myTopic != "home/lights/bedroom/lighter/brightness") {
        lastMessage = 1;
        if (newMessage == "1") {
            if (firstBrightness == 0.0) {
                brightness = 0.5;
                lasthue = 100;
                lastsaturation = 50;
                firstBrightness = 1.0;                
            }
            if (tempBrightness != 0.0) {
                brightness = tempBrightness;
            }              
            if (brightness > 0.999 && lamp == "OFF") {
                digitalWrite(LIGHT_SWICH_PIN, HIGH);
                lamp = "ON";
            }    
            while (lastbrightness <= brightness) {
                for(i=0; i<strip.numPixels(); i++) {
                    lastbrightness += 0.002;
                    strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
            }
            lastbrightness = brightness;                   
        }
        if (newMessage == "0") {
            brightness = 0.0;
            tempBrightness = lastbrightness;
            if (lamp == "ON" && brightness <= 0.99) {
                digitalWrite(LIGHT_SWICH_PIN, LOW);
                lamp = "OFF";
            }              
            while (lastbrightness >= brightness) {                                
                lastbrightness -= 0.002;
                for(i=0; i<strip.numPixels(); i++) {
                    strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
            }
            lastbrightness = brightness;     
        }
        lastMessage = 0;
    }
    // ----------------- Brightness up to 1.0 -----------------
    if (myTopic == "home/lights/bedroom/lighter/brightness" && lastMessage == 0) { 
        lastMessage = 1;
        brightness = newMessage.toFloat()/100;
            if (brightness >= 1.0) {
                digitalWrite(LIGHT_SWICH_PIN, HIGH);
                light = "ON";
            }                 
            if (brightness > lastbrightness) {
                while (lastbrightness <= brightness) {
                    lastbrightness += 0.001;  
                    for(i=0; i<strip.numPixels(); i++) {
                        strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                    }
                    strip.show();      
                }
                // lastbrightness = brightness;
            } 
            if (brightness <= 0.99 && light == "ON") {
                digitalWrite(LIGHT_SWICH_PIN, LOW);
                light = "OFF";
            }                
            if (brightness < lastbrightness) {
                while (lastbrightness >= brightness) {
                    lastbrightness -= 0.001;
                    for(i=0; i<strip.numPixels(); i++) {
                        strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                    }
                    strip.show();
                }
                // lastbrightness = brightness;
            }   
        lastMessage = 0;           
    }
    // ------------------ Hue value 0-360 -----------------------
    if (myTopic == "home/lights/bedroom/lighter/hue" && lastMessage == 0) { 
        lastMessage = 1;
        hue = newMessage.toInt();
        if (hue >= lasthue) {
            while (lasthue <= hue) {
                for(i=0; i<strip.numPixels(); i++) {
                    strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
                lasthue += 1;
                delay(10); 
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
                delay(10);
            }
            lasthue = hue;      
        }
        lastMessage = 0;
    }        
    //-------------------- Saturation value at 0-100 ------------------------ 
    if (myTopic == "home/lights/bedroom/lighter/saturation" && lastMessage == 0)  {
        lastMessage = 1; 
        saturation = newMessage.toInt();
        if (saturation >= lastsaturation) {
            while (lastsaturation <= saturation) {
                for(i=0; i<strip.numPixels(); i++) {
                    strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
                lastsaturation += 1;
                delay(20);       
            }
            lastsaturation = saturation;
        }
        if (saturation <= lastsaturation) {
            while (lastsaturation >= saturation) {
                for(i=0; i<strip.numPixels(); i++) {
                    strip.setPixelColor(i, HSVColor(lasthue,lastsaturation,lastbrightness));
                }
                strip.show();
                lastsaturation -= 1;
                delay(20);        
            }
            lastsaturation = saturation;
        }
        lastMessage = 0;
    }       
} 

void tempH() {///////////////  TEMP & HUMI //////////            
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
    Serial.println("The table lighter in bedroom v.62 (18.10.2018)");
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
    // attempt to connect to Wifi network:   
    if (WiFi.status() != WL_CONNECTED) {
        colorWipe(strip.Color(0, 0, 0), 5);
        digitalWrite(LIGHT_SWICH_PIN, LOW);
        reconnect();
    }
    if (!client.connected()) {
        colorWipe(strip.Color(0, 0, 0), 5);
        digitalWrite(LIGHT_SWICH_PIN, LOW);
        reconnectMQTT();
    }
    // if (client.connected()){
    tempH();
    //analogSens();
    delay(100);
    client.loop();
         
}
