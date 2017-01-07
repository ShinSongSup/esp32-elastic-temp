#include <WiFi.h>
#include <TimeLib.h>
#include <String.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DS1302RTC.h>

// WiFi network name and password:
const char * networkName = "yours";
const char * networkPswd = "yours";

const int BUTTON_PIN = 0;
const int TEMP_PIN = 27;
const int LED_PIN = 5;
unsigned long previousMillis = 0;
const long interval = 5000;

OneWire oneWire(TEMP_PIN);

DallasTemperature sensors(&oneWire);

DS1302RTC RTC(14, 13, 16);

void connectToWiFi(const char * ssid, const char * pwd)
{
  int ledState = 0;

  printLine();
  Serial.println("Connecting to WiFi network: " + String(ssid));

  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) 
  {
    // Blink LED while we're connecting:
    digitalWrite(LED_PIN, ledState);
    ledState = (ledState + 1) % 2; // Flip ledState
    delay(250);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_PIN, HIGH);
  printLine();
}

void printLine()
{
  Serial.println();
  for (int i=0; i<30; i++)
    Serial.print("-");
  Serial.println();
}

float getTemp()
{
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

String zeroString(int i){
  String st;
  if (i < 10) {
    st = String("0") + i;
  } else {
    st = String(i);
  }
  return st;
}

void sendTemp()
{
  WiFiClient client;
  if (!client.connect("cthulhu", 9200)){
    Serial.println("Not connected");
    return;
  }
  
  tmElements_t tm;

  long epoch = RTC.get();

  if (! RTC.read(tm)) {
    float tempc = getTemp();

    String iso_time = String(tm.Year+1970) + "-" + zeroString(tm.Month) +
        "-" + zeroString(tm.Day) + "T"  + zeroString(tm.Hour) +
        ":" + zeroString(tm.Minute) + ":" + zeroString(tm.Second);

    String packet = String("{\"@timestamp\" : \"") + iso_time + "\", \"temp\" : " + tempc + "}\r\n\r\n";

    client.print(
             String("PUT /esp32/tempc/") + epoch + " HTTP/1.1\r\n" +
                    "Host: cthulhu\r\n" +
                    "Connection: close\r\n" +
                    "Accept: */*\r\n" +
                    "Content-Length: " + packet.length() + "\r\n" +
                    "Content-Type: application/x-www-form-urlencoded\r\n\r\n" +
                    packet
    );
    
    unsigned long timeout = millis();
    while (client.available() == 0) 
    {
      if (millis() - timeout > 5000) 
      {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
      }
    }
  
    // Read all the lines of the reply from server and print them to Serial
    while (client.available()) 
    {
      String line = client.readStringUntil('\r');
      //Serial.print(line);
    }
  
    //Serial.println();
    //Serial.println("closing connection");
    client.stop();
  } else {
    Serial.println("DS1302 read error!  Please check the circuitry.");
    Serial.println();
  }
}


void setup()
{
  // Initilize hardware:
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Connect to the WiFi network (see function below loop)
  connectToWiFi(networkName, networkPswd);

  if (RTC.haltRTC()) {
    Serial.println("The DS1302 is stopped.  Please run the SetTime");
    Serial.println("example to initialize the time and begin running.");
    Serial.println();
  }
  if (!RTC.writeEN()) {
    Serial.println("The DS1302 is write protected. This normal.");
    Serial.println();
  }
  setSyncProvider(RTC.get);
  sensors.begin();
}

void loop()
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendTemp();
  }

}


