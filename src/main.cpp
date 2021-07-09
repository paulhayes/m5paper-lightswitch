#include <M5EPD.h>
#include "./epdgui/epdgui.h"
#include <WiFi.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "./creds.h"

M5EPD_Canvas canvas(&M5.EPD);

const int interationTimeout = 10000;
const int maxButtons = 10;
EPDGUI_Button *btns[32];

String effectListAddress = "http://office-lights.lan/effects";
String selectEffectAddress = "http://office-lights.lan/effects/select/";
String *effectNames[32];
String *effectIds[32];
int numEffects = 0;

unsigned long lastTouchTime = 0;

void initWifi()
{
  // Connect to WiFi
  Serial.println("Setting up WiFi");
  WiFi.setHostname("m5-paper");
  WiFi.begin(WIFI_SSID, WIFI_PSK);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("Connected. IP=");
  Serial.println(WiFi.localIP());
  delay(1000);
}


void getEffects()
{
    HTTPClient http;    
    http.begin(effectListAddress.c_str());
    int httpResponseCode = http.GET();
    if(httpResponseCode==200){
        DynamicJsonDocument doc(1024);
        deserializeJson(doc,http.getString());
        JsonObject obj = doc.as<JsonObject>()["effects"].as<JsonObject>();
        for (JsonPair kv : obj) {
            
            effectNames[numEffects] = new String(kv.value().as<char*>());
            effectIds[numEffects] = new String(kv.key().c_str());
            numEffects++;
            Serial.println(*effectNames[numEffects-1]);
            Serial.println(*effectIds[numEffects-1]);
            if(numEffects==32){
                break;
            }
        }
    }
    http.end();
    Serial.println(httpResponseCode);
}

void selectEffect(epdgui_args_vector_t &args)
{
    HTTPClient http;
    String *id = (String *)(args[0]);
    http.begin((selectEffectAddress+*id).c_str());
    int httpResponseCode = http.GET();
    if(httpResponseCode==200){
        DynamicJsonDocument doc(1024);
        deserializeJson(doc,http.getString());        
    }
    http.end();
    Serial.println(httpResponseCode);
    lastTouchTime = millis();
}

void initView()
{
    
    SPIFFS.begin(true);
    M5.begin();
    M5.EPD.SetRotation(90);
    M5.TP.SetRotation(90);
    M5.EPD.Clear(true);
    M5.RTC.begin();
    
    canvas.createCanvas(540, 960);
    canvas.loadFont("/opensans-modified.ttf",SPIFFS);
    canvas.createRender(36, 256);

    int len = min(maxButtons,numEffects);
    for (int i = 0; i < len; i++)
    {
        Serial.print("Creating button ");
        Serial.println(i);
        btns[i] = new EPDGUI_Button(*effectNames[i], 12, 12 + i * (80+12), 540-2*12, 80);
        EPDGUI_AddObject(btns[i]);
        
        btns[i]->AddArgs(EPDGUI_Button::EVENT_RELEASED, 0, effectIds[i]);
        btns[i]->Bind(EPDGUI_Button::EVENT_RELEASED, selectEffect);
    }
}

void setup()
{
    initWifi();
    getEffects();
    initView();
}

void loop()
{
    EPDGUI_Run();
    unsigned long elapsed = millis() - lastTouchTime;
    if(elapsed>interationTimeout){
        setCpuFrequencyMhz(20);
        esp_sleep_enable_touchpad_wakeup();
        esp_light_sleep_start();

        setCpuFrequencyMhz(240);
        lastTouchTime = millis();
    }
}