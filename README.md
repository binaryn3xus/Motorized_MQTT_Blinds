# Motorized_MQTT_Blinds

### THIS IS A FORK OF ROB'S CODE FROM THE HOOK UP: [MOTORIZED MQTT BLINDS PROJECT](https://github.com/thehookup/Motorized_MQTT_Blinds/)

______________________________________________

## The Original Video
This repository is to accompany Rob's Motorized_MQTT_Blinds video:

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/1O_1gUFumQM/0.jpg)](https://www.youtube.com/watch?v=1O_1gUFumQM)

## Parts List
*Affiliate links to support Rob!*

Stepper Motors: https://amzn.to/2D5rVsF

Stepper Drivers: https://amzn.to/2OZqW1W

NodeMCU: https://amzn.to/2I89xDF

12V Power Supply: https://amzn.to/2G2ZJrf

Buck Converter: https://amzn.to/2UsQ7jA

## 3D Printing

Download the correct STL file(s) for your style of tilt rod. You will find all the 3D Print file in the `extra/files` folder.

## Wiring Schematic

![alt text](extra/images/Schematic.jpg)

**Dont forget to cut the center trace on the stepper motor as shown in the youtube video**

## How to setup, build, and flash this image

I recommend using VSCode with the PlatformIO extension. So these instructions will only support this method:

## First-Time Setupdfvb 
1) Clone the project or download/unzip the files from the browser
2) Open in VSCode
3) Rename `user_config.h.sample` to `user_config.h`
4) Fill out `user_config.h` with your information for your setup.
5) When you are ready to build, click on the PlatformIO icon in the left panel.
6) In the Project Tasks window, drill into `env:d1_mini`* > General > Click Build!

\* = While the project actually uses a Wemos D1 Mini, this code should still work with no issues on a NodeMCU like in the schematic picture.

You should leave "STEPS_TO_CLOSE" at 12 to start with.  It can be adjusted for your specific blinds

## Home Assistant YAML

Replace "BlindsMCU" with your MQTT_CLIENT_ID if you changed it in the file setup

```yaml
cover:
  - platform: mqtt
    name: "Motorized Blinds"
    command_topic: "BlindsMCU/blindsCommand"
    set_position_topic: "BlindsMCU/positionCommand"
    position_topic: "BlindsMCU/positionState"
    state_topic: "BlindsMCU/positionState"
    retain: true
    payload_open: "OPEN"
    payload_close: "CLOSE"
    payload_stop: "STOP"
    position_open: 0
    position_closed: 12
  ```

## KNOWN ISSUES:
* Blinds default to the `Open` position when they reboot for some reason.
  * This is my main reason for forking this project. So expect it to be fixed soon hopefully.
  
## Recommended Tools

Ender3 3d Printer: https://amzn.to/2GcznnZ

Dupont Crimper and Connector Set: https://amzn.to/2X1Oeap

## Unlike Rob, I do not support an Alexa based code-base.
