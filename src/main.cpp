/*
 * To control sleep time with data from zivyobraz.eu :)
 * For eample for Air Wick® Auto Spray 
 * or for anythig! you need to turn on in specific time intervals.
 * 
 * Default password for Wi-Fi AP is: zivyobraz
 * 
 * Code forked from:<
 * https://github.com/MultiTricker/zivyobraz-fw
 * 
 * Libraries:
 * WiFi manager by tzapu https://github.com/tzapu/WiFiManager
 * SHT4x (temperature, humidity): https://github.com/adafruit/Adafruit_SHT4X
 * BME280 (temperature, humidity, pressure): https://github.com/adafruit/Adafruit_BME280_Library
 * SCD41 (CO2, temperature, humidity): https://github.com/sparkfun/SparkFun_SCD4x_Arduino_Library
 *
 * original code made by @MultiTricker (https://michalsevcik.eu)
 * modified by LaskaKit @laska_kit
 */

/////////////////////////////////
// Uncomment for correct board
/////////////////////////////////
#define microESP

#ifdef microESP
#define PIN_CH0     7     // Channel 0 GPIO on Laskakit microESP board with MOSFET Shield v1.1 and above
#define PIN_CH1     1     // Channel 1 GPIO on Laskakit microESP board with MOSFET Shield 
#define PIN_CH2     4     // Channel 2 GPIO on Laskakit microESP board with MOSFET Shield
#define PIN_SDA     8    // SDA pin for I2C
#define PIN_SCL     10    // SCL pin for I2C

#else
  #error "Board not defined!"
#endif

////////////////////////////
// Library etc. includes
////////////////////////////

// WiFi
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>

// Supported sensors
#ifdef SENSOR
  // SHT40/41/45
  #include "Adafruit_SHT4x.h"
  Adafruit_SHT4x sht4 = Adafruit_SHT4x();

  // SCD40/41
  #include "SparkFun_SCD4x_Arduino_Library.h"
  SCD4x SCD4(SCD4x_SENSOR_SCD41);

  // BME280
  #include <Adafruit_Sensor.h>
  #include <Adafruit_BME280.h>
  Adafruit_BME280 bme;
#endif

/* ---- ADC reading - indoor Battery voltage ---- */
#ifdef microESP
  #define vBatPin 0
  #define dividerRatio 1.7693877551
#endif

/* ---- Server Zivy obraz ----------------------- */
const char *host = "cdn.zivyobraz.eu";
const char *firmware = "2.3";
const String wifiPassword = "zivyobraz";

/* ---------- Deepsleep time in minutes --------- */
uint64_t defaultDeepSleepTime = 2; // if there is a problem with loading images,
                                  // this time will be used as fallback to try again soon
uint64_t deepSleepTime = defaultDeepSleepTime; // actual sleep time in minutes, value is changed
                                              // by what server suggest in response headers
/* ---------------------------------------------- */

/* variables */
String ssid; // Wi-Fi ssid
int8_t rssi; // Wi-Fi signal strength
float d_volt; // indoor battery voltage
RTC_DATA_ATTR uint64_t timestamp = 0;
RTC_DATA_ATTR uint8_t notConnectedToAPCount = 0;
uint64_t timestampNow = 1; // initialize value for timestamp from server

// Function to perform the main tasks
void doTheThings() {
  // Add the main tasks here
  Serial.println("Performing the main tasks...");

  for(int i = 0; i <= 2; i++){   
    // changing the LED brightness with PWM
    ledcWrite(0, 255);
    delay(100);
    ledcWrite(1, 255);
    delay(100);
    ledcWrite(2, 255);
    delay(1000);
    ledcWrite(0, 0);
    delay(100);
    ledcWrite(1, 0);
    delay(100);
    ledcWrite(2, 0);
    delay(1000);
  }
}

const String getWifiSSID() {
  String wifiSSID = WiFi.SSID();
  Serial.println("Wifi SSID: " + wifiSSID);

  // Replace special characters
  wifiSSID.replace("%", "%25");
  wifiSSID.replace(" ", "%20");
  wifiSSID.replace("#", "%23");
  wifiSSID.replace("$", "%24");
  wifiSSID.replace("&", "%26");
  wifiSSID.replace("'", "%27");
  wifiSSID.replace("(", "%28");
  wifiSSID.replace(")", "%29");
  wifiSSID.replace("*", "%2A");
  wifiSSID.replace("+", "%2B");
  wifiSSID.replace(",", "%2C");
  wifiSSID.replace("/", "%2F");
  wifiSSID.replace(":", "%3A");
  wifiSSID.replace(";", "%3B");
  wifiSSID.replace("=", "%3D");
  wifiSSID.replace("?", "%3F");
  wifiSSID.replace("@", "%40");
  wifiSSID.replace("[", "%5B");
  wifiSSID.replace("]", "%5D");

  return wifiSSID;
}

int8_t getWifiStrength() {
  int8_t rssi = WiFi.RSSI();
  Serial.println("Wifi Strength: " + String(rssi) + " dB");

  return rssi;
}

float getBatteryVoltage() {
  float volt;

  #ifdef microESP
    volt = analogReadMilliVolts(vBatPin) * dividerRatio / 1000;
  #endif

  Serial.println("Battery voltage: " + String(volt) + " V");

  return volt;
}

// This is called if the WifiManager is in config mode (AP open)
// and draws information screen
void configModeCallback(WiFiManager *myWiFiManager) {
  // Iterate notConnectedToAPCount counter
  if(notConnectedToAPCount < 255)   {
    notConnectedToAPCount++;
  }
}

void WiFiInit() {
  // Connecting to WiFi
  Serial.println();
  Serial.print("Connecting... ");
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setWiFiAutoReconnect(true);
  wm.setConnectRetries(5);
  wm.setDarkMode(true);
  wm.setConnectTimeout(5);
  wm.setSaveConnectTimeout(5);

  // Set network name to wi-fi mac address
  String hostname = "VUNE_";
  hostname += WiFi.macAddress();
  // Replace colon with nothing
  hostname.replace(":", "");

  // reset settings - wipe stored credentials for testing
  //wm.resetSettings();

  wm.setConfigPortalTimeout(240); // set portal time to 4 min, then sleep/try again.
  wm.setAPCallback(configModeCallback);
  wm.autoConnect(hostname.c_str(), wifiPassword.c_str());

  // Check if Wi-Fi is connected
  if (WiFi.status() == WL_CONNECTED) {
    // Reset counter    
    notConnectedToAPCount = 0;
  }
}

bool createHttpRequest(WiFiClient &client, bool &connStatus, bool checkTimestamp, const String &extraParams) {
  // Make an url
  String url = "index.php?mac=" + WiFi.macAddress() +
              (checkTimestamp ? "&timestamp_check=1" : "") +
              "&rssi=" + String(rssi) +
              "&ssid=" + ssid +
              "&v=" + String(d_volt) +
              "&fw=" + String(firmware) +
              "&vonavka=1" +
              "&ap_retries=" + String(notConnectedToAPCount) +
              extraParams;

  Serial.print("connecting to ");
  Serial.println(host);

  // Let's try twice
  for (uint8_t client_reconnect = 0; client_reconnect < 3; client_reconnect++) {
    if (!client.connect(host, 80)) {
      Serial.println("connection failed");
      if (client_reconnect == 2) {
        deepSleepTime = defaultDeepSleepTime;
        if (!checkTimestamp) return false;
        delay(500);
      }
      if (!checkTimestamp) delay(200);
    }
  }

  Serial.print("requesting URL: ");
  Serial.println(String("http://") + host + "/" + url);
  client.print(String("GET ") + "/" + url + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "Connection: close\r\n\r\n");
  Serial.println("request sent");

  // Workaroud for timeout
  uint32_t timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      if (checkTimestamp) deepSleepTime = defaultDeepSleepTime;
      return false;
    }
  }

  bool gotTimestamp = false;

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    //Serial.println(line);

    if (checkTimestamp) {
      // If line starts with "Timestamp", put it into the timestamp variable
      if (line.startsWith("Timestamp")) {
        gotTimestamp = true;
        // Skipping also colon and space - also in following code for sleep, rotate, ...
        timestampNow = line.substring(11).toInt();
        Serial.print("Timestamp now: ");
        Serial.println(timestampNow);
      }

      // Let's try to get info about how long to go to deep sleep
      if (line.startsWith("Sleep")) {
        uint64_t sleep = line.substring(7).toInt();
        deepSleepTime = sleep;
        Serial.print("Sleep: ");
        Serial.println(sleep);
      }
    }

    if (!connStatus) {
      connStatus = line.startsWith("HTTP/1.1 200 OK");
      Serial.println(line);
    }
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }

  // Is there a problem? Fallback to default deep sleep time to try again soon
  if (!connStatus) {
    deepSleepTime = defaultDeepSleepTime;
    return false;
  }

  // For debug purposes - print out the whole response
  /*
  Serial.println("Byte by byte:");

  while (client.connected() || client.available()) {
    if (client.available()) {
      char c = client.read();  // Read one byte
      Serial.print(c);         // Print the byte to the serial monitor
    }
  }
  client.stop();
  /* */

  if (checkTimestamp) {
    if (gotTimestamp && (timestampNow == timestamp)) {
      Serial.print("No screen reload, because we already are at current timestamp: ");
      Serial.println(timestamp);
      return false;
    }

    // Set timestamp to actual one
    timestamp = timestampNow;
  }

  return true;
}

#ifdef SENSOR
int readSensorsVal(float &sen_temp, int &sen_humi, int &sen_pres) {
Wire.begin (PIN_SDA, PIN_SCL);
delay(100);   
  // Check for SHT40 OR SHT41 OR SHT45
  if (sht4.begin()) {
    Serial.println("SHT4x FOUND");
    sht4.setPrecision(SHT4X_LOW_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);

    sensors_event_t hum, temp;
    sht4.getEvent(&hum, &temp);

    sen_temp = temp.temperature;
    sen_humi = hum.relative_humidity;
    return 1;
  }

  // Check for BME280
  if (bme.begin()) {
    Serial.println("BME280 FOUND");

    sen_temp = bme.readTemperature();
    sen_humi = bme.readHumidity();
    sen_pres = bme.readPressure() / 100.0F;
    return 2;
  }

  // Check for SCD40 OR SCD41
  if (SCD4.begin(false, true, false)) {
    Serial.println("SCD4x FOUND");
    SCD4.measureSingleShot();

    while (SCD4.readMeasurement() == false) { // wait for a new data (approx 30s)
      Serial.println("Waiting for first measurement...");
      delay(1000);
    }

    sen_temp = SCD4.getTemperature();
    sen_humi = SCD4.getHumidity();
    sen_pres = SCD4.getCO2();
    return 3;
  }

  return 0;
}
#endif


bool checkForNewTimestampOnServer(WiFiClient &client) {
  bool connection_ok = false;
  String extraParams = "";

  // Measuring temperature and humidity?
  #ifdef SENSOR
    #if (defined ESPink_V2)   // Leave it here for some new boards, wich might needs to power up uSup
    // LaskaKit ESPink 2.5 needs to power up uSup
    setEPaperPowerOn(true);
    delay(50);
    #endif

    float temperature;
    int humidity;
    int pressure;
    uint8_t sen_ret = readSensorsVal(temperature, humidity, pressure);

    if (sen_ret) {
      extraParams = "&temp=" + String(temperature) + "&hum=" + String(humidity);

      switch (sen_ret) {
        case 2:
          extraParams += "&pres=" + String(pressure); // BME280
          break;
        case 3:
          extraParams += "&co2=" + String(pressure); // SCD4x
          break;
      }
    }

    #if (defined ESPink_V2)   // Leave it here for some new boards, wich might needs to shut down uSup
    // Power down for now
    setEPaperPowerOn(false);
    #endif
  #endif

  return createHttpRequest(client, connection_ok, true, extraParams);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting firmware for Zivy Obraz service");

  ledcSetup(0, 1000, 8);
  ledcAttachPin(PIN_CH0, 0);
  ledcSetup(1, 1000, 8);
  ledcAttachPin(PIN_CH1, 1);
  ledcSetup(2, 1000, 8);
  ledcAttachPin(PIN_CH2, 2);

  doTheThings(); // Call the function to perform the main tasks

  // Battery voltage measurement
  d_volt = getBatteryVoltage();

  // Wifi init
  WiFiInit();

  // WiFi strength - so you will know how good your signal is
  rssi = getWifiStrength();

  // WiFi SSID - get connected ssid
  ssid = getWifiSSID();

  // Use WiFiClient class to create TCP connections
  WiFiClient client;

  // Successfully connected to Wi-Fi?
  if(notConnectedToAPCount == 0) {
    // Load timestamp
    checkForNewTimestampOnServer(client);
    // Let's spray the senteces
    // TODO fuction for this
    Serial.println("Connected to Wi-Fi, going to sleep for " + String(deepSleepTime) + " minutes.");
    delay(100);
  } else {
    Serial.println("No Wi-Fi connection, will sleep for a while and try again. Failure no.: " + String(notConnectedToAPCount));

    // Determine how long we will sleep determined by number of notConnectedToAPCount
    if(notConnectedToAPCount <= 3) deepSleepTime = defaultDeepSleepTime;
    else if(notConnectedToAPCount <= 10) deepSleepTime = 10;
    else if(notConnectedToAPCount <= 20) deepSleepTime = 30;
    else if(notConnectedToAPCount <= 50) deepSleepTime = 60;
    else deepSleepTime = 720;
  }

  // Deep sleep mode
  Serial.print("Going to sleep now for (minutes): ");
  Serial.println(deepSleepTime);

  esp_sleep_enable_timer_wakeup(deepSleepTime * 60 * 1000000);
  delay(100);
  esp_deep_sleep_start();
}

void loop() {}