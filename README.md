
# Freetronics Relay 8 Mqtt Control

PLEASE NOTE THIS PROGRAM IS CURRENTLY BEING REWRITTEN IT MAY BE BROKEN.

Copyright 2020 Bedrock Media Productions Pty Ltd www.bedrockmediaproductions.com.au

MQTT SCRIPT FOR CONTROLLING RELAY8 SHIELDS FROM FREETRONICS

Features:

This project aims to use an Arduino to trigger relay drivers
* attached via I2C by messages received from MQTT

##MQTT Topics

We use a prefix to create out topic this can be change in the configuration section,
By default the command topic structure is house/switchboard/12/relay/+/command (+ being the relay number) this program currently supports up to 32 relays
By default the status topic structure is house/switchboard/12/relay/+/status (+ being the relay number) this program currently supports up to 32 relays



More information:

Relay Carrier Shield <https://www.superhouse.tv/product/relay-shield-carrier-x4/>

Relay8 Watchdog Shield <https://bedrockmediaproductions.com.au/product/r8wds/>

 ## CREDITS 
Written by 
* Jon Oxer - Copyright 2015-2017 SuperHouse Automation Pty Ltd <info@superhouse.tv>
* Alex Ferrara <alex@receptiveit.com.au>
* James Kennewell - Copyright 2019 Bedrock Media Productions Pty Ltd <james@bedrockmediaproductions.com.au>

## DISTRIBUTION
The specific terms of distribution of this project are governed by the license referenced below.

##LICENCE
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License  along with this program.  If not, see <http://www.gnu.org/licenses/>.
