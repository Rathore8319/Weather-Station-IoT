#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// DHT 
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Wifi vars
const char* ssid = "Redmi 12 5G";
const char* password = "1234567890";

// Server vars
String url = "https://weatherserver-o7pj.onrender.com";

// Project vars
String data; String checkin; String checkout; int active = 0; String WeatherjsonData; String jsonData;
int valueArray[2][2] = {{1,0},{0,0}};
unsigned long previousMillis = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(ssid,password);
  Serial.print("Connecting!");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  if(WiFi.status() == WL_CONNECTED) {
    delay(3000);
    checkActive(url);
    if(active == 1) {
      float temp = dht.readTemperature();
      float humidity = dht.readHumidity();
      weather(url, temp, humidity);
    }
  }
}

void loop() {
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  weather(url, temp, humidity);

  // Running active status!
  unsigned long currentMillis = millis();
  if((currentMillis - previousMillis) >= 180000) {
    previousMillis = currentMillis;
    checkActive(url);
  }
}

void checkActive(String url) {
  String curUrl = url + "/statusG";
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(curUrl);
    int code = http.GET();
    if(code > 0) {
      checkin = http.getString();
      Serial.println(checkin);

      // Accessing checkIN data
      DynamicJsonDocument checkIndoc(228);
      deserializeJson(checkIndoc, checkin);
      String check = checkIndoc[0]["statusCheck"];
      if(check == "areyouactive?") {
        http.end();
        String newUrl = url + "/statusR";
        http.begin(newUrl);
        jsonData = "{\"statusReceive\": \"yes\"}";
        http.addHeader("Content-Type", "application/json");
        int newCode = http.PATCH(jsonData);
        if(newCode > 0) {
          Serial.println("Status sent");
          String res = http.getString();
          Serial.println(res);
          active = 1;
        } else {
          Serial.println("Error sending status");
        }
      } else if(check == "Ok") {
        active = 1;
      }
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void weather(String url, float temp, float humidity) {
  String weatherUrl = url + "/weather";
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(weatherUrl);
    int intTemp = (int) temp;
    int intHumid = (int) humidity;
    valueArray[1][0] = intTemp;
    valueArray[1][1] = intHumid;
    if((valueArray[0][0] == valueArray[1][0]) && (valueArray[0][1] == valueArray[1][1])) {
      valueArray[0][0] = temp; valueArray[0][1] = humidity; valueArray[1][0] = 0; valueArray[1][1] = 0;
      Serial.println("Same data");
    } else {
      Serial.println("Different data");
      valueArray[0][0] = temp; valueArray[0][1] = humidity; valueArray[1][0] = 0; valueArray[1][1] = 0;
      WeatherjsonData = "{\"temp\": ";
      WeatherjsonData.concat(temp);
      WeatherjsonData.concat(",\"humidity\":");
      WeatherjsonData.concat(humidity);
      WeatherjsonData.concat("}");
      Serial.println(WeatherjsonData);
      
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.PATCH(WeatherjsonData);
      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
      } else {
        Serial.print("Error on sending PATCH: ");
        Serial.println(httpResponseCode);
      }
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}
