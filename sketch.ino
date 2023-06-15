#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <unordered_map>

const int chipSelect = D8;  // Pin D8 is the default chip select pin for the SD card shield
String ssid = "";
const char* password = "";

ESP8266WebServer server(80);
unsigned long startTime = 0;
unsigned long webserverActiveValue = 0;

void setup() {
  Serial.begin(19200);
  while (!Serial)
    ;
  Serial.println("\n");
  Serial.println("Hello World");

  // Initialize SD card
  Serial.println("Initialize SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialization ok.");

  // Read WebserverActive value from config.txt
  Serial.println("Reading config.txt...");
  File configFile = SD.open("/config.txt", FILE_READ);
  if (!configFile) {
    Serial.println("Failed to open config.txt file!");
    return;
  }

  while (configFile.available()) {
    String line = configFile.readStringUntil('\n');
    line.trim();
    Serial.println("reading line: " + line);

    int separatorIndex = line.indexOf('=');
    if (separatorIndex == -1) {
      continue;
    }

    String paramName = line.substring(0, separatorIndex);
    String paramValue = line.substring(separatorIndex + 1);

    //process configuration parameter values for the server
    if (paramName == "WebserverActive") {
      webserverActiveValue = paramValue.toInt() * 60000;
      Serial.print("webserverActiveValue calculated value set to: ");
      Serial.print(webserverActiveValue);
      Serial.println(" seconds.");
    }
    if (paramName == "SSID") {
      ssid = paramValue;
      Serial.print("ssid value set to: ");
      Serial.print(ssid);
      Serial.println(" ");
    }
    // Process other configuration parameters as needed
    // ...
  }

  configFile.close();

  // Set up Wi-Fi access point
  Serial.println("Setting up Access Point and webserver...");
  IPAddress ip(192, 168, 1, 1);        // Set the desired IP address for the access point
  IPAddress subnet(255, 255, 255, 0);  // Set the desired subnet mask
  IPAddress gateway(192, 168, 1, 1);   // Set the desired gateway address

  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(ssid, password);

  // Wait until the IP address is set
  Serial.println("Wait until the IP configuration is set...");
  while (WiFi.softAPIP().toString() == "0.0.0.0") {
    delay(100);  // Adjust the delay duration as needed
  }
  Serial.print("Local IP address: ");
  Serial.println(WiFi.softAPIP());

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/reboot", handleReboot);
  Serial.println("Starting webserver...");
  server.begin();

  startTime = millis();  // Start the timer
  Serial.println("Set Up Done.");
}

void loop() {
  server.handleClient();

  // Check if the time has exceeded the webserverActiveValue
  if (millis() - startTime >= webserverActiveValue) {
    Serial.println("Stopping server.");
    server.stop();
    WiFi.softAPdisconnect(true);
    while (true) {
      // Stay in an infinite loop or perform any other desired actions after shutting down the server and access point
    }
  }
}

void handleRoot() {
  Serial.println("webserver: handleRoot");
  File configFile = SD.open("/config.txt", FILE_READ);
  if (!configFile) {
    server.send(500, "text/plain", "Failed to open config.txt file!");
    return;
  }

  String html = "<html><body>";
  html += "<style>";
  html += "@media (max-width: 600px) {";  // Apply styles for screens up to 600px wide
  html += "  form { max-width: 100%; padding: 0 10px; }";
  html += "}";
  html += "@media (min-width: 601px) {";  // Apply styles for screens wider than 600px
  html += "  form { max-width: 500px; margin: 0 auto; }";
  html += "}";
  html += "</style>";
  html += "<form action='/save' method='POST'>";


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

  // Add the reboot button to the HTML form
  html += "<input type='submit' value='Save'></form>";
  html += "<form action='/reboot' method='POST' style='margin-top: 20px;'>";
  html += "<input type='submit' value='Restart Device'></form>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleSave() {
  Serial.println("webserver: handleSave");
  // Step 1: Check if the file "config.tmp" exists. If it does, delete it.
  if (SD.exists("/config.txt")) {
    Serial.println("remove config.txt");
    SD.remove("/config.txt");
  }

  // Step 2: Create a new file called "config.txt".
  File configFile = SD.open("/config.txt", FILE_WRITE);
  Serial.println("write config.txt");
  if (!configFile) {
    server.send(500, "text/plain", "Failed to create config.txt file!");
    return;
  }

  String response = "Configuration saved successfully!\nNew values:\n";

  // Step 3: Write the new parameter/value pairs to the "config.txt" file.
  Serial.println("Write the new parameter/value pairs to config.txt");
  for (int i = 0; i < server.args(); i++) {
    String paramName = server.argName(i);
    String paramValue = server.arg(i);

    //handle actuals
    if (paramName == "WebserverActive") {
      if (paramValue.toInt() < 5) {
        paramValue = String(5);
      }
    }

    // Update the parameter values with the new ones provided through the web interface
    Serial.println(paramName + "=" + paramValue);
    configFile.println(paramName + "=" + paramValue);
    response += paramName + ": " + paramValue + "\n";
  }

  configFile.close();
  Serial.println("closed config.txt");

  server.sendHeader("Location", String("/"), true);  // Redirect to the configuration page
  server.send(302, "text/plain", response);          // Send redirect response
}

void handleReboot() {
  Serial.println("webserver: handleReboot");
  // Perform any necessary actions before rebooting

  // Reboot the device
  ESP.restart();
}
