
# Relay8 MQTT Driver Software

Copyright 2020 Bedrock Media Productions Pty Ltd www.bedrockmediaproductions.com.au

MQTT SCRIPT FOR CONTROLLING RELAY8 SHIELDS FROM FREETRONICS

Features:

This project aims to use an Arduino to trigger relay drivers
* attached via I2C by messages received from MQTT

##MQTT Topics

We use a single-level wildcard to set the topic, The default the command topic structure is "house/switchboard/01/relay/+/command" (+ being the relay number) 
this code allow tor triggering of the Relay driver boards Jon Oxer  designed years ago (Relay8) it uses a single mqtt topic and pulls the relay number out.  so all you need to have in the payload is a ``1`` for ON or a ``0`` for OFF 

More infomation to be added soon






More information:

Relay Carrier Shield <https://www.superhouse.tv/product/relay-shield-carrier-x4/>

Relay8 Watchdog Shield <https://bedrockmediaproductions.com.au/product/r8wds/>

 ## CREDITS 
Written by 
* Jon Oxer               - Copyright 2015-2017 SuperHouse Automation Pty Ltd <info@superhouse.tv>
* James Kennewell        - Copyright 2019-2020 Bedrock Media Productions Pty Ltd <james@bedrockmediaproductions.com.au>
* Alex Ferrara           - <alex@receptiveit.com.au>
* Chris Aitken @aitken85 - SuperHouse Automation Discord Server
* Lusa         @lusa     - SuperHouse Automation Discord Server

## DISTRIBUTION
The specific terms of distribution of this project are governed by the license referenced below.

##LICENCE
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License  along with this program.  If not, see <http://www.gnu.org/licenses/>.
