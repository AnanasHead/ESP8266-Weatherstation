#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
// Verwendete Bilder einbinden
#include "weather.h"
#include "signal.h"

//DHT Pin Belegung 
#define DHT_TYPE  DHT11
#define DHT_PIN   D5
#define DHT_POWER D0

//LED Pins
#define LED_ROT   D8
#define LED_GRUEN D6
#define LED_GELB  D7

//DHT Sensor 
DHT dht(DHT_PIN, DHT_TYPE);

//Display-Attribute definieren
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 0
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Werte des Bildes
#define IMG_WIDTH  32 
#define IMG_HEIGHT 35

const long interval = 2000;

const long utcOffsetInSeconds = 3600; // 7200 bei Sommerzeit
// Wochentage
String weekDays[7] = {"So.", "Mo.", "Di.", "Mi.", "Do.", "Fr.", "Sa."};
//Monate
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//Daten des WLAN Netzes
const char* ssid = "AnanasNetz";//"UBBST-WLAN";
const char* password = "54669285510875438452";//"Game2016";

int Timeout = 0;
int progress = 0;

String currentDate;

//Helligkeitssensor
const int SENSOR = 0;

WiFiServer server(80);

void setup(void) {
  // put your setup code here, to run once:
  pinMode(DHT_POWER, OUTPUT);
  pinMode(LED_ROT, OUTPUT);
  pinMode(LED_GRUEN, OUTPUT);
  pinMode(LED_GELB, OUTPUT);
  digitalWrite(DHT_POWER, HIGH);

  Serial.begin(115200);
  delay(500);
  // Aufbereitung des Displays
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  //display.setFont(&DejaVu_Serif_9);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  delay(1000);

  // Vorbereitung der WiFi-Verbindung
  if (ssid != "" || password != ""){
      display.println("Verbinde zu: "); //Ausgabe der SSID auf der Seriellen Schnittstelle.
      display.println(ssid);
      progress = 10; 
      drawProgressbar(0,40,120,10, progress ); // declare the graphics fillrect(int x, int y, int width, int height)
      display.display();
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED && Timeout <= 30) { // Warten auf Verbindung
            delay(100);
            drawProgressbar(0,40,120,10, progress ); // declare the graphics fillrect(int x, int y, int width, int height)
            display.display();
            Serial.print(".");
            Timeout++;
            progress = progress + 3;
     }
     if(WiFi.status() == WL_CONNECTED){
          display.setCursor(0,0);
          display.clearDisplay();
          //Erfolgreiche Verbindung
          display.print("Mit ");
          display.print(ssid);
          display.print(" verbunden\n");
          server.begin();
          timeClient.begin();
          display.display();
          delay(1500);
     }
     else
     {
       display.setCursor(0,0);
       display.clearDisplay();
       display.println("Verbindung fehlgeschlagen");
       display.display();
       delay(2000);
     }
  }
  //DHT Sensor starten
  dht.begin();

  // Aufbereitung des Displays
  delay(2000);
  display.clearDisplay();
}

void loop(void) {
  //Vorbereitung des Display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();

  drawHeader();

  float h = getHumidity();
  float t = getTemp(false); // Temperatur in °C
  float f = getTemp(true); // Temperatur in °F

  //Ausgabe auf dem Display
  display.println("LF: " + (String)h + "%");
  display.println("T:  " + (String)t + char(247) + "C");
  display.println("T:  " + (String)f + char(247) + "F");
  getNetworkRSSI();
  if (t < 10.00){
    display.drawBitmap(95, 12, snow, IMG_WIDTH, IMG_HEIGHT, WHITE);
  }

  // Ausgabe auf dem Seriellen Monitor (Debug)
  Serial.println("Luftfeuchtigkeit: " + (String)h + " % ");
  Serial.println("Temperatur      : " + (String)t + " °C");
  Serial.println("Temperatur      : " + (String)f + " F ");
  Serial.println();

  //Steuerung der LED´s über die Luftfeuchtigkeit
  if (h < 60.00)
  {
    digitalWrite(LED_ROT, LOW);
    digitalWrite(LED_GELB, LOW);
    digitalWrite(LED_GRUEN, HIGH);
  }else if (h > 60.00 && h < 70.00) {
    digitalWrite(LED_GRUEN, LOW);
    digitalWrite(LED_ROT, LOW);
    digitalWrite(LED_GELB, HIGH);
  }else if (h > 70.00) {
    digitalWrite(LED_GRUEN, LOW);
    digitalWrite(LED_GELB, LOW);
    digitalWrite(LED_ROT, HIGH);
    display.drawBitmap(95, 12, wet, IMG_WIDTH, IMG_HEIGHT, WHITE);
  }

  //Logik des Helligkeitssensors
  //Anzeige unterschiedlicher Bilder anhand der Helligkeit
  if (analogRead(SENSOR) < 300) {
   //display.println("Es ist dunkel!");
   Serial.println("Es ist dunkel!");
   display.drawBitmap(65, 12, moon, IMG_WIDTH, IMG_HEIGHT, WHITE);
  }
  else {
   //display.println("Es ist hell!");
   Serial.println("Es ist hell!");
   display.drawBitmap(65, 12, sun, IMG_WIDTH, IMG_HEIGHT, WHITE);
  }

  drawFooter();  
  display.display();

  if(WiFi.status() == WL_CONNECTED){
      //Nur Daten senden wenn ein Client verbunden ist
      checkClientCon();
  }
  delay(interval);
}

// Modus = false °C || Modus = true °F
float getTemp(bool Modus) {
  float temperature = dht.readTemperature(Modus);
  if (isnan(temperature)) {
    Serial.println("Fehler beim auslesen");
    temperature = -1;
  }
  return temperature;
}
//Luftfeuchtigkeit auslesen
float getHumidity() {
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    Serial.println("Fehler beim auslesen");
    humidity = -1;
  }
  return humidity;
}

 void checkClientCon(){
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  delay(1000);
  if (client.available()) {
      Serial.println("Client verbunden.");
      sendRespond(client);
    }
    client.flush();
  }

  void drawHeader(){
    display.drawLine(0, 10, 128, 10, WHITE);
    display.setCursor(28, 0);
    if(WiFi.status() == WL_CONNECTED){
      display.print(WiFi.localIP());
      if (getNetworkRSSI() < -30 && getNetworkRSSI() > -60)
          display.drawBitmap(110, -5, signal_good, IMG_WIDTH, IMG_HEIGHT, WHITE);
      else if (getNetworkRSSI() <= -60 && getNetworkRSSI() > -80){
          display.drawBitmap(110, -5, signal_mid, IMG_WIDTH, IMG_HEIGHT, WHITE);
      }
      else if (getNetworkRSSI() <= -80){
          display.drawBitmap(110, -5, signal_bad, IMG_WIDTH, IMG_HEIGHT, WHITE);
      }
    }else{
      display.setCursor(20, 0);
      display.print("Nicht verbunden");  
    }
    display.setCursor(0, 15);
  }

  void drawFooter(){
    //Update der Zeit über Internet
    timeClient.update();
    //Ausgabe des aktuellen Datum
    getCurrentDate();
    // Serial Ausgabe für Debug
    Serial.println(timeClient.getFormattedTime());
    // Ausgabe der Uhrzeit und des Wochentages auf dem Display
    display.drawLine(0, 52, 128, 52, WHITE);
    display.setCursor(0, 56);
    display.println(timeClient.getFormattedTime());
    display.setCursor(70, 56);
    display.println(currentDate);
  }

  void drawProgressbar(int x,int y, int width,int height, int progress) // Abgeleitet von "https://steemit.com/utopian-io/@lapilipinas/arduino-blog-no-12-how-to-make-a-progress-bar-using-oled-display-and-rotary-encoder"
  {
   progress = progress > 100 ? 100 : progress; // set the progress value to 100
   progress = progress < 0 ? 0 :progress; // start the counting to 0-100
   float bar = ((float)(width-1) / 100) * progress;
   display.drawRect(x, y, width, height, WHITE);
   display.fillRect(x+2, y+2, bar , height-4, WHITE); // initailize the graphics fillRect(int x, int y, int width, int height)
  }

 void sendRespond(WiFiClient client){  
   //JSON Aufbau
   client.println("HTTP/1.1 200 OK");
   client.println("Content-Type: application/json");
   client.println("");
   client.print("{");
   client.print("\"temperatur\":");
   client.print("{");
   client.print("\"celsius\":");
   client.print(getTemp(false));
   client.print(",");
   client.print("\"fahrenheit\":");
   client.print(getTemp(true));
   client.print("},");
   client.print("\"humidity\":");
   client.print(getHumidity());
   client.print(",\"time\":");
   client.print('"');
   client.print(timeClient.getFormattedTime());
   client.print('"');
   client.print(",\"date\":");
   client.print('"');
   client.print(currentDate);
   client.print('"');
   client.print(",\"rssi\":");
   client.print('"');
   client.print(getNetworkRSSI());
   client.print('"');
   client.println("}");  
 }

 long getNetworkRSSI(){
  //Auslesen des Wifi RSSI(Received Signal Strength Indicator)
  long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(rssi);
  return rssi;
}

void getCurrentDate(){ // Abgeleitet von "https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/"
  time_t epochTime = timeClient.getEpochTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);

  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime); 

  int monthDay = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(monthDay);

  int currentMonth = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(currentMonth);

  String currentMonthName = months[currentMonth-1];
  Serial.print("Month name: ");
  Serial.println(currentMonthName);

  int currentYear = ptm->tm_year+1900;
  Serial.print("Year: ");
  Serial.println(currentYear);

  //Print complete date:
  currentDate = String(monthDay) + "." + String(currentMonth) + "." + String(currentYear);
  Serial.print("Current date: ");
  Serial.println(currentDate);
}
