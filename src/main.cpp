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
PubSubClient client(espClient);
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
SimpleTimer timer;
AH_EasyDriver shadeStepper(STEPPER_STEPS_PER_REV, STEPPER_DIR_PIN, STEPPER_STEP_PIN, STEPPER_MICROSTEP_1_PIN, STEPPER_MICROSTEP_2_PIN, STEPPER_SLEEP_PIN);

//Global Variables
bool boot = true;
int currentPosition = 0;
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

//Functions
void setup_wifi()
{
  Serial.println(__cplusplus);
  // We start by connecting to a WiFi network
  Serial.println("Connecting to " + String(ssid));
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  deviceIpAddress = WiFi.localIP();
  Serial.println("WiFi connected! IP address: " + deviceIpAddress.toString());
}

void reconnect()
{
  int retries = 0;
  while (!client.connected())
  {
    if (retries < 150)
    {
      Serial.print("Attempting MQTT connection...");
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass))
      {
        Serial.println("connected");
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
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    if (retries > 149)
    {
      ESP.restart();
    }
  }
}

char *IntToChar(int currentPosition)
{
  char *ch = new char;
  String temp_str = String(currentPosition);
  temp_str.toCharArray(ch, temp_str.length() + 1);
  return ch;
}

void SetPosition(unsigned int positionNum)
{
  if (positionNum <= STEPS_TO_CLOSE && positionNum >= (STEPS_TO_CLOSE*(-1)))
  {
    auto positionCharArray = IntToChar(positionNum);
    client.publish(USER_MQTT_CLIENT_NAME "/positionCommand", positionCharArray, false);
  }
  else
  {
    Serial.write("Invalid range for 'positionCommand'");
  }
}

void Close()
{
  SetPosition(STEPS_TO_CLOSE);
}

void Stop()
{
  SetPosition(currentPosition);
}

void Open()
{
  SetPosition(0);
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
      Open();
    }
    else if (newPayload == "CLOSE")
    {
      Close();
    }
    else if (newPayload == "STOP")
    {
      Stop();
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
  Serial.println(currentPosition);
  Serial.println(newPosition);
}

void checkIn()
{
  client.publish(USER_MQTT_CLIENT_NAME "/checkIn", "OK");
}

String GetDeviceDetailsHtml()
{
  String htmlString = "<html><head><title>" + String(USER_MQTT_CLIENT_NAME) + "</title></head><body><p>";
  htmlString += "<strong>MQTT Client Name:</strong> " + String(USER_MQTT_CLIENT_NAME) + "<br/>";
  htmlString += "<strong>MQTT Server:</strong> " + String(USER_MQTT_SERVER) + ":" + String(USER_MQTT_PORT) + "<br/>";
  htmlString += "<strong>Assigned Network SSID:</strong> " + String(USER_SSID) + "<br />";
  htmlString += "<strong>IP Address:</strong> " + deviceIpAddress.toString() + "<br />";
  htmlString += "<strong>Location:</strong> " + String(LOCATION) + "<br />";
  htmlString += "<p><a href=" + String(OPEN_WEBUI_PATH) + ">Open Blinds</a></p>";
  htmlString += "<p><a href=" + String(CLOSE_WEBUI_PATH) + ">Close Blinds</a></p>";
  htmlString += "<p><a href=\"" + String(OTAPATH) + "\">Go To Update Firmware</a></p>";
  htmlString += "</body></html>";
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

//Run once setup
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
  WiFi.mode(WIFI_AP_STA);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  httpServer.on("/", update_status);
  httpServer.on(OPEN_WEBUI_PATH, WebOpenBlinds);
  httpServer.on(CLOSE_WEBUI_PATH, WebCloseBlinds);
  MDNS.begin(USER_MQTT_CLIENT_NAME);
  httpUpdater.setup(&httpServer, OTAPATH);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", USER_MQTT_CLIENT_NAME);

  delay(10);
  timer.setInterval(((1 << STEPPER_MICROSTEPPING) * 5800) / STEPPER_SPEED, processStepper);
  timer.setInterval(90000, checkIn);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  httpServer.handleClient();
  MDNS.update();
  timer.run();
}
