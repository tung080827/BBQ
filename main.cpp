#include <WiFi.h>
#include "bbq_comp.h"
#include <Preferences.h>
#include "html.h"
#include <SPI.h>
#include "MAX31865.h"

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

#define MOSI_PIN 23
#define MISO_PIN 19
#define SCK_PIN 18
// #define SS_PIN  5
#define SS_PIN 2




Preferences preferences;
MAX318650 pt100(SS_PIN, MOSI_PIN, MISO_PIN, SCK_PIN);
int highValue = -1;
int lowValue = -1;
int readyValue = -1;
int currentValue = -1;

// String ssid = "BBQ-Network";
// String password = "bbq-network";
String ssid = "Lento 502"; // Place your home wifi ssid
String password = "staylento"; //// Place your home wifi password
String ap_ssid = "BBQ-Network";
String ap_password = "bbq-network";
String mode = "1";
int sOn = false;
int lOn = false;

// WiFiServer server(80);
WiFiServer webServer(80);

String header;

// Current time
unsigned long currentTime = millis();
unsigned long currentread = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds
const long timeoutTime = 500;

int testWifi(int timeretries)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while (c < timeretries)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WiFi connected.");
      Serial.println(WiFi.localIP());
      return (1);
    }
    if (c >= timeretries)
    {
      break;
    }
    delay(500);
    Serial.println(WiFi.status());
    c++;
  }
  return (0);
} //end testwifi

void setupAP(void)
{
  WiFi.mode(WIFI_STA); //mode STA
  WiFi.disconnect();   //disconnect to scan wifi
  delay(100);

  Serial.println("");
  delay(100);
  WiFi.softAP(ap_ssid, ap_password); //change to AP mode with AP ssid and APpass
  Serial.println("softAP");
  Serial.println("");
  Serial.println(WiFi.softAPIP());
  // if(setAPcount < 1)
  // {
  
  // }
}

void setup()
{
    Serial.begin(9600);

    double temp = pt100.getTemperature();
    Serial.print(temp);

    preferences.begin("bbqx", false);

    ssid = preferences.getString("ssid", ssid);
    ap_ssid = preferences.getString("ap_ssid", ap_ssid);
    password = preferences.getString("pass", password);
    ap_password = preferences.getString("ap_password", ap_password);

    highValue = preferences.getInt("highValue", highValue);
    lowValue = preferences.getInt("lowValue", lowValue);
    readyValue = preferences.getInt("readyValue", readyValue);
    sOn = preferences.getBool("sOn", sOn);
    lOn = preferences.getBool("lOn", lOn);

    Serial.println("");
    Serial.println(ssid);
    Serial.println(password);
    Serial.println("");
    Serial.println(ap_ssid);
    Serial.println(ap_password);
    // Check mode
    
  if (testWifi(20)) /*--- if the stored SSID and password connected successfully, exit setup ---*/
  {
    Serial.println("Station mode");
    delay(1000);
    mode = "STA";
  }
  else /*--- otherwise, set up an access point to input SSID and password  ---*/
  {

    Serial.println("Connect timed out, opening AP");
   
    delay(2000);
    setupAP();
    mode = "AP";
    Serial.println("Working on AP mode");
  }


    if (mode == "STA"){
        Serial.println("Working on STA mode");
    }

    webServer.begin();
    init_liquid_level();
    // init_pt100();
    // if(mode == "STA"){
       
    // }
    
}

void loop()
{
    WiFiClient client = webServer.available(); // Listen for incoming clients

    if (client)
    { // If a new client connects,
        currentTime = millis();
        previousTime = currentTime;
        Serial.println("New Client."); // print a message out in the serial port
        String currentLine = "";       // make a String to hold incoming data from the client

        while (client.connected() && currentTime - previousTime <= timeoutTime)
        {
            // loop while the client's connected
            currentTime = millis();
            // currentread = millis();
            if (client.available())
            {

                String html = mainPage;
                if (millis() - currentread > 500)
                {
               
                    currentValue = pt100.getTemperature();
                    currentread = millis();
                    if(mode == "STA"){
                        //futher function
                    }
                   
                }

                html.replace("&&current", String(currentValue));

                // if there's bytes to read from the client,
                char c = client.read(); // read a byte, then
                // Serial.write(c);        // print it out the serial monitor
                header += c;
                if (c == '\n')
                { // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        if ((header.indexOf("/getSensorValue") != -1) || (header.indexOf("/16/getSensorValue") != -1))
                        {
                            //    Serial.println("getSensorValue");
                            // Sensör değeri alımını burada gerçekleştirin
                            // float sensorValue = ; // Sensör verilerini okuyan işlev

                            // İstemciye sensör değerini gönder
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-Type: text/plain");
                            client.println("Connection: close");
                            client.println();
                            if (sOn)
                            {
                                client.print(currentValue); // Sensör değerini yanıt olarak gönder
                            }
                            else
                            {
                                client.print("Alarm Closed"); // Sensör değerini yanıt olarak gönder
                            }
                            break;
                        }

                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();
                        // render introductions  page
                        if ((header.indexOf("GET /16/introductions") >= 0) || (header.indexOf("GET /introductions") >= 0))
                        {
                            String introductionHtml = introductionPage;
                            Serial.println("introductions");
                            introductionHtml.replace("&&style", style);
                            client.println(introductionHtml);
                            break;
                        }
                        // render settings page
                        if ((header.indexOf("GET /16/settings") >= 0) || (header.indexOf("GET /settings") >= 0))
                        {
                            String settingsHtml = settingsPage;
                            Serial.println("Settings page method get");
                            settingsHtml.replace("&&style", style);
                            settingsHtml.replace("&&pwd", password);
                            settingsHtml.replace("&&ssid", ssid);
                            // settingsHtml.replace("&&mode", ssid);

                            client.println(settingsHtml);
                            break;
                        }
                        if ((header.indexOf("GET /16/savesettings") >= 0) || (header.indexOf("GET /savesettings") >= 0))
                        {
                            Serial.println("Settings page method get");
                            Serial.println(header);

                            String newSsid = getQueryParams(header, "ssid=", "&");
                            newSsid.replace("+"," ");
                            String newPwd = getQueryParams(header, "pwd=", " ");
                            // String newmode = getQueryParams(header, "mode=", "&");

                            String settingsHtml = settingsPage;

                            settingsHtml.replace("&&style", style);
                            settingsHtml.replace("&&ssid", newSsid);
                            settingsHtml.replace("&&pwd", newPwd);

                            preferences.putString("ssid", newSsid);
                            preferences.putString("pass", newPwd);
                            // preferences.putString("mode", newmode);
                            Serial.println(newSsid);
                            Serial.println(newPwd);
                            // Serial.println(newmode);

                            client.println(settingsHtml);
                            break;
                        }

                        // STILL TEMP KAPATILDI
                        if ((header.indexOf("GET /16/sOff") >= 0) || (header.indexOf("GET /sOff") >= 0))
                        {
                            Serial.println("sOff");
                            sOn = false;
                            preferences.putBool("sOn", sOn);
                        }
                        // STILL TEMP ACILDI
                        else if ((header.indexOf("GET /16/sOn") >= 0) || (header.indexOf("GET /sOn") >= 0))
                        {
                            Serial.println("sOn");
                            sOn = true;
                            preferences.putBool("sOn", sOn);
                        }
                        // LIQUD KAPATILDI
                        else if ((header.indexOf("GET /16/lOff") >= 0) || (header.indexOf("GET /lOff") >= 0))
                        {
                            Serial.println("lOff");
                            lOn = false;
                            preferences.putBool("lOn", lOn);
                        }
                        // LIQUD ACILDI
                        else if ((header.indexOf("GET /16/lOn") >= 0) || (header.indexOf("GET /lOn") >= 0))
                        {
                            Serial.println("lOn");
                            lOn = true;
                            preferences.putBool("lOn", lOn);
                        }
                        // RESTART
                        else if ((header.indexOf("GET /16/restart") >= 0) || (header.indexOf("GET /restart") >= 0))
                        {
                            Serial.println("restart");
                            ESP.restart();
                        }
                        else if ((header.indexOf("GET /16/wifiSetup") >= 0) || (header.indexOf("GET /wifiSetup") >= 0))
                        {
                            Serial.println("wifiSetup");
                            // WİFİ AYARLAR SAYFASINA GÖNDER
                        }

                        else if ((header.indexOf("GET /16/set") >= 0) || (header.indexOf("GET /set") >= 0))
                        {
                            Serial.println("header");
                            Serial.println(header);
                            Serial.println("");

                            String high = getQueryParams(header, "high=", "&");
                            String low = getQueryParams(header, "low=", "&");
                            String ready = getQueryParams(header, "ready=", " ");

                            Serial.print("High:");
                            Serial.println(high);
                            Serial.print("Low:");
                            Serial.println(low);
                            Serial.print("Ready:");
                            Serial.println(ready);
                            highValue = high.toInt();
                            lowValue = low.toInt();
                            readyValue = ready.toInt();

                            preferences.putInt("highValue", highValue);
                            preferences.putInt("lowValue", lowValue);
                            preferences.putInt("readyValue", readyValue);
                        }

                        /// SAYFAYI YAZDIR
                        html.replace("&&style", style);
                        if (sOn == true)
                        {
                            html.replace("&&sButton", sOffButton);
                        }
                        else
                        {
                            html.replace("&&sButton", sOnButton);
                        }

                        if (lOn == true)
                        {
                            html.replace("&&lButton", lOffButton);
                        }
                        else
                        {
                            html.replace("&&lButton", lOnButton);
                        }

                        html.replace("&&high", String(highValue));
                        html.replace("&&low", String(lowValue));
                        html.replace("&&ready", String(readyValue));
                        // Serial.println("");
                        // Serial.println("H, L, R");
                        // Serial.println(highValue);
                        // Serial.println(lowValue);
                        // Serial.println(readyValue);
                        // Serial.println("");

                        html.replace("&&script", script);
                        client.println(html);
                        // Serial.println("html");
                        // Serial.println(html);
                        // Serial.println("");

                        break;
                    }
                    else
                    { // if you got a newline, then clear currentLine
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {                     // if you got anything else but a carriage return character,
                    currentLine += c; // add it to the end of the currentLine
                }
            }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
    }
}
