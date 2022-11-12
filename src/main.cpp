// COMP-10184 – Mohawk College
// WebServer and FileSystem
//
// This program...
//
// @author  Damanpreet Singh
// @id   000741359
//
// I created this work and I have not shared it with anyone else.
#include <Arduino.h>
#include <LittleFS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
// *********************************************************************
// constants
#define VERSION "1.0"

// access credentials for WiFi network.
const char *ssid = "#####";
const char *password = "######";

// web server hosted internally to the IoT device.  Listen on TCP port 80,
// which is the default HTTP port.
ESP8266WebServer webServer(80);

// Pin that the  DS18B20 is connected to
const int oneWireBus = D3;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature DS18B20(&oneWire);

// *********************************************************************
// simple handler that just returns program version to client
void handleVersion()
{
  String sText;

  sText = VERSION;
  sText += "\n";
  webServer.send(200, "text/plain", "Version: " + sText);
}
// Sensor address variable
DeviceAddress thermometer;

String convertAddress(DeviceAddress deviceAddress)
{
  // Convert the address to a string
  char addressString[17];
  for (uint8_t i = 0; i < 8; i++)
  {
    if (thermometer[i] < 16)
      sprintf(addressString + (i * 2), "0");
    sprintf(addressString + (i * 2), "%X", thermometer[i]);
  }

  return(addressString);
}

// *********************************************************************
// this function examines the URL from the client and based on the extension
// determines the type of response to send.
bool loadFromLittleFS(String path)
{
  bool bStatus;
  String contentType;

  // set bStatus to false assuming this will not work.
  bStatus = false;

  // assume this will be the content type returned, unless path extension
  // indicates something else
  contentType = "text/html";

  // DEBUG:  print request URI to user:
  Serial.print("Requested URI: ");
  Serial.println(path.c_str());

  // if no path extension is given, assume index.html is requested.
  if (path.endsWith("/"))
    path += "index.html";

  // look at the URI extension to determine what kind of data to
  // send to client.

  // try to open file in LittleFS filesystem
  File dataFile = LittleFS.open(path.c_str(), "r");
  // if dataFile <> 0, then it was opened successfully.
  if (dataFile)
  {
    if (webServer.hasArg("download"))
      contentType = "application/octet-stream";
    // stream the file to the client.  check that it was completely sent.
    if (webServer.streamFile(dataFile, contentType) != dataFile.size())
    {
      Serial.println("Error streaming file: " + String(path.c_str()));
    }
    // close the file
    dataFile.close();
    // indicate success
    bStatus = true;
  }

  return bStatus;
}

// *********************************************************************
void handleWebRequests()
{
  if (!loadFromLittleFS(webServer.uri()))
  {
    // file not found.  Send 404 response code to client.
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += webServer.uri();
    message += "\nMethod: ";
    message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += webServer.args();
    message += "\n";
    for (uint8_t i = 0; i < webServer.args(); i++)
    {
      message += " NAME:" + webServer.argName(i) + "\n VALUE:" + webServer.arg(i) + "\n";
    }
    webServer.send(404, "text/plain", message);
    Serial.println(message);
  }
}


void readTemperature()
{
  if (webServer.method() == HTTP_GET)
  {

    DS18B20.getAddress(thermometer, 0);
    String buffer = convertAddress(thermometer);
    // Get temps
    DS18B20.requestTemperatures();

    webServer.send(200, "application/json", "{\"temperature\": " + String(DS18B20.getTempCByIndex(0)) + ", \"address\": \"" + buffer + "\"}");
  }
    
}

void setup()
{
  Serial.begin(115200);
  DS18B20.begin();
  Serial.println("\nWeb Server and FileSystem");

  // Initialize File System
  LittleFS.begin();
  Serial.println("File System Initialized");

  // attempt to connect to WiFi network
  Serial.printf("\nConnecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  webServer.begin();

  Serial.printf("\nWeb server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());

  webServer.on("/temperature", readTemperature);
  webServer.on("/version", handleVersion);
  webServer.onNotFound(handleWebRequests);


}
void loop()
{
  webServer.handleClient();
}
