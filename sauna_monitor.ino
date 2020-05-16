#include <TFT_eSPI.h>
//#include <SPI.h>
#include "WiFi.h"
//#include <Wire.h>
//#include "esp_adc_cal.h"
//#include "bmp.h"
#include <Button2.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 27
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23

#define TFT_BL          4   // Display backlight control pin
#define ADC_EN          14  //ADC_EN is the ADC detection enable port
#define ADC_PIN         34
#define BUTTON_1        35
#define BUTTON_2        0

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

char buff[512];
int vref = 1100;
int btnCick = false;

const char* ssid = "TV";
const char* password = "greenday219";
const char* serverName = "http://schizo:3000/";

unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 10000;
float temp = 0;

Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);
int btnClick = false;
int messageSent = false;
unsigned long messageTime = 0;
//unsigned long messageDelay = 600000; // 10 minutes
unsigned long messageDelay = 1000 * 60 * 60 * 6; // 10 minutes

void button_init()
{
    btn1.setPressedHandler([](Button2 & b) {
        btnClick = true;
        Serial.println("Wifi information.");
        wifi_info();
    });

    btn2.setPressedHandler([](Button2 & b) {
        btnClick = false;
        Serial.println("Temp monitor");
        temp_monitor();
    });
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}

void wifi_info()
{
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("Network info", tft.width() / 2, tft.height() / 2 - 32);
    String wifi = WiFi.localIP().toString();
    tft.drawString("IP Address: " + wifi, tft.width() / 2, tft.height() / 2 - 16);
    delay(3000);
    btnClick = false;
    tft.fillScreen(TFT_BLACK);
}

void connect() {
  Serial.print("Connecting to Wifi Network: ");
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Connecting to Wifi Network:", tft.width() / 2 - 64, tft.height() / 2 );
  delay(500);
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  String progress = ".";
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(progress, tft.width() / 2 - 64, tft.height() / 2 + 12);
    progress += ".";
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected to WiFi.");
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Connected to WiFi.", tft.width() / 2 - 64, tft.height() / 2 + 24);
  tft.setTextDatum(TL_DATUM);
  delay(2000);
  Serial.println("IP address of ESP32 is : ");
  Serial.println(WiFi.localIP());
}

//void send_event(const char *event)

void updateScreen(const float temp)
{
  float val = temp / 100;
  String degrees = String(val, 2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Sauna temp monitor", tft.width() / 2 - 56, tft.height() / 2 - 56 );  
  tft.setTextSize(3);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Temp: ", tft.width() / 2 - 56, tft.height() / 2 );

  if (val < 50.0) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK); 
  }
  else if (val < 70.0) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK); 
  }
  else {
    tft.setTextColor(TFT_RED, TFT_BLACK); 
  }
  tft.drawString(degrees + "C", tft.width() / 2 + 60, tft.height() / 2 );
  tft.setTextDatum(TL_DATUM);
}

void sendTemp(float temp, String path)
{
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      http.begin(String(serverName) + path);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(String("{\"temp\": \"") + temp + String("\"}"));
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected\n Trying ro reconnect");
      connect();
    }
}

void temp_monitor()
{   
    if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
      temp = (int) (sensors.getTempCByIndex(0) * 100.0);
      String textTemp = String(temp, 0);
      Serial.print("Temperature is: ");
      Serial.println(textTemp);
      updateScreen(temp);
      sendTemp(temp, "temp/");
    lastTime = millis();
  }
  if (temp > 8000.00) {
    if (!messageSent) {
      Serial.println("Sending notification!");
      sendTemp(temp, "send/");
      messageSent = true;
      messageTime = millis();
    }
    else if (messageSent && (millis() - messageTime) > messageDelay) {
      Serial.println("Sending notification again");
      messageSent = true;
      messageTime = millis();
    }
  }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Start");
    sensors.begin();
    button_init();
    tft.init();
    tft.setRotation(1);
    connect();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    /*
    ADC_EN is the ADC detection enable port
    If the USB port is used for power supply, it is turned on by default.
    If it is powered by battery, it needs to be set to high level
    */
    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);
    updateScreen(0);
}


void loop()
{
  sensors.requestTemperatures();
  if (btnClick) {
     wifi_info();
  }
  else {
    temp_monitor();
  }
  button_loop();
}
