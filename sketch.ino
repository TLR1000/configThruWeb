#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <unordered_map>

const int chipSelect = D8;  // Pin D8 is the default chip select pin for the SD card shield
const char* ssid = "LazyG";
const char* password = "";

ESP8266WebServer server(80);
unsigned long startTime = 0;
unsigned long webserverActiveValue = 0;

void handleRoot() {
  File configFile = SD.open("/config.txt", FILE_READ);
  if (!configFile) {
    server.send(500, "text/plain", "Failed to open config.txt file!");
    return;
  }

  String html = "<html><body>";
  html += "<form action='/save' method='POST'>";  // Add form element

  while (configFile.available()) {
    String line = configFile.readStringUntil('\n');
    line.trim();

    if (line.length() == 0 || line.startsWith("#")) {
      continue;
    }

    html += "<label>" + line + "</label><br>";
    String paramName = line.substring(0, line.indexOf('='));
    html += "<input type='text' name='" + paramName + "' value='" + line.substring(line.indexOf('=') + 1) + "'><br><br>";
  }
  configFile.close();

  html += "<input type='submit' value='Save'></form></body></html>";

  server.send(200, "text/html", html);
}


#include <SD.h>

void handleSave() {
  // Step 1: Check if the file "config.tmp" exists. If it does, delete it.
  if (SD.exists("/config.tmp")) {
    SD.remove("/config.tmp");
  }

  // Step 2: Create a new file called "config.tmp".
  File configFile = SD.open("/config.tmp", FILE_WRITE);
  if (!configFile) {
    server.send(500, "text/plain", "Failed to create config.tmp file!");
    return;
  }

  String response = "Configuration saved successfully!\nNew values:\n";

  // Step 3: Write the new parameter/value pairs to the "config.tmp" file.
  for (int i = 0; i < server.args(); i++) {
    String paramName = server.argName(i);
    String paramValue = server.arg(i);

    // Update the parameter values with the new ones provided through the web interface
    if (paramName == "WebserverActive") {
      webserverActiveValue = paramValue.toInt();
    }

    configFile.println(paramName + "=" + paramValue);

    // Append new values to the response string
    response += paramName + ": " + paramValue + "\n";
  }

  configFile.close();

  // Step 4: Check if the file "config.txt" exists. If it does, rename it to "config.old".
  if (SD.exists("/config.txt")) {
    SD.rename("/config.txt", "/config.old");
  }

  // Step 5: Rename the file "config.tmp" to "config.txt".
  SD.rename("/config.tmp", "/config.txt");

  server.sendHeader("Location", String("/"), true); // Redirect to the configuration page
  server.send(302, "text/plain", response); // Send redirect response
}





void setup() {
  Serial.begin(9600);

  // Initialize SD card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }

  // Read WebserverActive value from config.txt
  File configFile = SD.open("/config.txt", FILE_READ);
  if (!configFile) {
    Serial.println("Failed to open config.txt file!");
    return;
  }

  while (configFile.available()) {
    String line = configFile.readStringUntil('\n');
    line.trim();

    if (line.length() == 0 || line.startsWith("#")) {
      continue;
    }

    int separatorIndex = line.indexOf('=');
    if (separatorIndex == -1) {
      continue;
    }

    String paramName = line.substring(0, separatorIndex);
    String paramValue = line.substring(separatorIndex + 1);

    if (paramName == "WebserverActive") {
      webserverActiveValue = paramValue.toInt();
    }

    // Process other configuration parameters as needed
    // ...
  }

  configFile.close();

  // Set up Wi-Fi access point
  WiFi.softAP(ssid, password);

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/save", handleSave);

  server.begin();

  startTime = millis(); // Start the timer
}

void loop() {
  server.handleClient();

  // Check if the time has exceeded the webserverActiveValue
  if (millis() - startTime >= webserverActiveValue) {
    server.stop();
    WiFi.softAPdisconnect(true);
    while (true) {
      // Stay in an infinite loop or perform any other desired actions after shutting down the server and access point
    }
  }
}
