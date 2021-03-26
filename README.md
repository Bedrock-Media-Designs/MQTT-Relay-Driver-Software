# MQTT-Relay-Driver-Software
Copyright 2020 Bedrock Media Productions Pty Ltd www.bedrockmediaproductions.com.au


ARDUINO SCRIPT FOR RECEIVING COMMANDS OVER MQTT AND PUSHING TO A RELAY DRIVER BOARD

## Features:

* Single Arduino to listen to MQTT topic with a wildcard, Publish the state change to a relay output chanel over I2C.
* Sends MQTT status payloads as JSON formatted string for easy parsing within your favourite automation software.
* Listens on a single topic with a wild card `house/switchboard/01/relay/+`
* Simple config.h file to edit your network and mqtt details, with main code ready to go without modification.
* Configured to allow for multiple relay controllers.

## Expected payloads
#### On (re)connection to MQTT broker:

`{event: reconnected, ip: ###.###.###.###, controller: #}`


#### On heartbeat:

`{event: status, ip: ###.###.###.###, controller: #}`

## Required libraries
* PubSubClient [https://github.com/knolleary/pubsubclient](https://github.com/knolleary/pubsubclient)
* Adafruit_MCP23017 [https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library](https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library)

## CREDITS
Written by 
* Jon Oxer               - Copyright 2015-2017 SuperHouse Automation Pty Ltd <info@superhouse.tv>
* James Kennewell        - Copyright 2019-2021 Bedrock Media Productions Pty Ltd <james@bedrockmediaproductions.com.au>
* Chris Aitken @aitken85 - SuperHouse Automation Discord Server | github.com/caitken-com

## DISTRIBUTION
The specific terms of distribution of this project are governed by the license referenced below.

## LICENCE
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.
