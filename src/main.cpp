#include <SimpleTimer.h>             //https://github.com/marcelloromani/Arduino-SimpleTimer/tree/master/SimpleTimer
#include <ESP8266WiFi.h>             //if you get an error here you need to install the ESP8266 board manager
#include <ESP8266mDNS.h>             //if you get an error here you need to install the ESP8266 board manager
#include <PubSubClient.h>            //https://github.com/knolleary/pubsubclient
#include <ESP8266WebServer.h>        // For Web-based Updates
#include <ESP8266HTTPUpdateServer.h> // For Web-based Updates
#include <AH_EasyDriver.h>           //http://www.alhin.de/arduino/downloads/AH_EasyDriver_20120512.zip
#include "user_config.h"             //user defined variables

WiFiClient espClient;
IPAddress deviceIpAddress;
String deviceMacAddress;
PubSubClient client(espClient);
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
SimpleTimer timer;
AH_EasyDriver shadeStepper(STEPPER_STEPS_PER_REV, STEPPER_DIR_PIN, STEPPER_STEP_PIN, STEPPER_MICROSTEP_1_PIN, STEPPER_MICROSTEP_2_PIN, STEPPER_SLEEP_PIN);

// Global Variables
bool boot = true;
int currentPosition = 0;
int reconnectRetries = 0;
int maxReconnectRetries = 150;
int newPosition = 0;
char positionPublish[50];
bool moving = false;
char charPayload[50];

const char *ssid = USER_SSID;
const char *password = USER_PASSWORD;
const char *mqtt_server = USER_MQTT_SERVER;
const int mqtt_port = USER_MQTT_PORT;
const char *mqtt_user = USER_MQTT_USERNAME;
const char *mqtt_pass = USER_MQTT_PASSWORD;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME;

// Functions
void setup_wifi()
{
  // We start by connecting to a WiFi network
  Serial.println("Connecting to " + String(ssid));
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  deviceIpAddress = WiFi.localIP();
  deviceMacAddress = WiFi.macAddress();
  Serial.println("\nWiFi connected! IP address: " + deviceIpAddress.toString());
}

void reconnect()
{
  if (!client.connected())
  {
    Serial.println("Attempting MQTT connection [" + String(reconnectRetries) + "] " + String(mqtt_user) + "@" + String(USER_MQTT_SERVER) + "...");
    if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass))
    {
      Serial.println("Connected to MQTT server!");
      if (boot == false)
      {
        client.publish(USER_MQTT_CLIENT_NAME "/checkIn", "Reconnected");
      }
      if (boot == true)
      {
        client.publish(USER_MQTT_CLIENT_NAME "/checkIn", "Rebooted");
      }
      // ... and resubscribe
      client.subscribe(USER_MQTT_CLIENT_NAME "/blindsCommand");
      client.subscribe(USER_MQTT_CLIENT_NAME "/positionCommand");
    }
    else
    {
      Serial.println("Connection failed, rc=" + String(client.state()) + "; Trying again in 5 seconds");
      reconnectRetries++; // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.printf("Message arrived [%s]", topic);
  String newTopic = topic;
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  int intPayload = newPayload.toInt();
  Serial.println(newPayload);
  newPayload.toCharArray(charPayload, newPayload.length() + 1);
  if (newTopic == USER_MQTT_CLIENT_NAME "/blindsCommand")
  {
    if (newPayload == "OPEN")
    {
      client.publish(USER_MQTT_CLIENT_NAME "/positionCommand", "0", true);
    }
    else if (newPayload == "CLOSE")
    {
      int stepsToClose = STEPS_TO_CLOSE;
      String temp_str = String(stepsToClose);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME "/positionCommand", charPayload, true);
    }
    else if (newPayload == "STOP")
    {
      String temp_str = String(currentPosition);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME "/positionCommand", positionPublish, true);
    }
  }
  if (newTopic == USER_MQTT_CLIENT_NAME "/positionCommand")
  {
    if (boot == true)
    {
      newPosition = intPayload;
      currentPosition = intPayload;
      boot = false;
    }
    if (boot == false)
    {
      newPosition = intPayload;
    }
  }
}

void processStepper()
{
  if (newPosition > currentPosition)
  {
#if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepON();
#endif
#if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepOFF();
#endif
    shadeStepper.move(80, FORWARD);
    currentPosition++;
    moving = true;
  }
  if (newPosition < currentPosition)
  {
#if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepON();
#endif
#if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepOFF();
#endif
    shadeStepper.move(80, BACKWARD);
    currentPosition--;
    moving = true;
  }
  if (newPosition == currentPosition && moving == true)
  {
#if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepOFF();
#endif
#if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepON();
#endif
    String temp_str = String(currentPosition);
    temp_str.toCharArray(positionPublish, temp_str.length() + 1);
    client.publish(USER_MQTT_CLIENT_NAME "/positionState", positionPublish);
    moving = false;
  }
  // Serial.println("Current Position: [" + String(currentPosition) + "]");
  // Serial.println("New Position: [" + String(newPosition) + "]");
}

void checkIn()
{
  client.publish(USER_MQTT_CLIENT_NAME "/checkIn", "OK");
}

String GetDeviceDetailsHtml()
{
  String htmlString = "<html><head><title>" + String(USER_MQTT_CLIENT_NAME) + "</title></head><body style='background-color:#1e1e1e; color:#FFF'><table><tbody>";
  htmlString += "<tr><th colspan='2'><h1>MQTT Info</h1></th></tr>";
  htmlString += "<tr><td><strong>MQTT Client Name</strong></td><td>" + String(USER_MQTT_CLIENT_NAME) + "</td></tr>";
  htmlString += "<tr><td><strong>MQTT Server</strong></td><td>" + String(USER_MQTT_SERVER) + ":" + String(USER_MQTT_PORT) + "</td></tr>";
  htmlString += "<tr><th colspan='2'><h1>---<br />Network Info</h1></th></tr>";
  htmlString += "<tr><td><strong>Assigned Network SSID</strong></td><td>" + String(USER_SSID) + "</td></tr>";
  htmlString += "<tr><td><strong>IP Address</strong></td><td>" + deviceIpAddress.toString() + "</td></tr>";
  htmlString += "<tr><td><strong>MAC Address</strong></td><td>" + deviceMacAddress + "</td></tr>";
  htmlString += "<tr><th colspan='2'><h1>---<br />Details & Actions</h1></th></tr>";
  htmlString += "<tr><td><strong>Location</strong></td><td>" + String(LOCATION) + "</td></tr>";
  htmlString += "<tr><td colspan='2'><a href=" + String(OPEN_WEBUI_PATH) + " style='color:#FFF;text-decoration:underline;'>Open Blinds</a></td></tr>";
  htmlString += "<tr><td colspan='2' style='color:#FFF;text-decoration:underline;'><a href=" + String(CLOSE_WEBUI_PATH) + " style='color:#FFF;text-decoration:underline;'>Close Blinds</a></td></tr>";
  htmlString += "<tr><td colspan='2' style='color:#FFF;text-decoration:underline;'><a href=\"" + String(OTAPATH) + "\" style='color:#FFF;text-decoration:underline;'>Go To Update Firmware</a></td></tr>";
  htmlString += "</tbody></table></body></html>";
  return htmlString;
}

void update_status()
{
  httpServer.send(200, "text/html", GetDeviceDetailsHtml());
}

void WebOpenBlinds()
{
  Serial.write("Sending Open Command from WebUI...");
  newPosition = 12;
  httpServer.send(200, "text/html", "Sent Open Command");
}

void WebCloseBlinds()
{
  Serial.write("Sending Close Command from WebUI...");
  newPosition = 0;
  httpServer.send(200, "text/html", "Sent Close Command");
}

// Run once setup
void setup()
{
  Serial.begin(115200);
  shadeStepper.setMicrostepping(STEPPER_MICROSTEPPING); // 0 -> Full Step
  shadeStepper.setSpeedRPM(STEPPER_SPEED);              // set speed in RPM, rotations per minute
#if DRIVER_INVERTED_SLEEP == 1
  shadeStepper.sleepOFF();
#endif
#if DRIVER_INVERTED_SLEEP == 0
  shadeStepper.sleepON();
#endif
  reconnectRetries = 0;
  WiFi.mode(WIFI_AP_STA);
  setup_wifi();
  httpServer.on("/", update_status);
  httpServer.on(OPEN_WEBUI_PATH, WebOpenBlinds);
  httpServer.on(CLOSE_WEBUI_PATH, WebCloseBlinds);
  httpUpdater.setup(&httpServer, OTAPATH);
  httpServer.begin();
  MDNS.begin(USER_MQTT_CLIENT_NAME);
  MDNS.addService("http", "tcp", 80);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println("HTTPUpdateServer ready! Open http://" + deviceIpAddress.toString() + OTAPATH + " in your browser");

  delay(10);
  timer.setInterval(((1 << STEPPER_MICROSTEPPING) * 5800) / STEPPER_SPEED, processStepper);
  timer.setInterval(90000, checkIn);
}

void loop()
{
  if (!client.connected())
  {
    if (reconnectRetries <= maxReconnectRetries)
    {
      reconnect();
    }
    else
    {
      Serial.println("Too many retry attempts [" + String(reconnectRetries) + " of " + String(maxReconnectRetries) + "], rebooting...");
      ESP.restart();
    }
  }
  client.loop();
  httpServer.handleClient();
  MDNS.update();
  timer.run();
}
