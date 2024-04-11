# Project Noisemeter Device

[Civic Tech TO](https://civictech.ca/)

## Overview

This repo contains all source code and design files for the Project Noisemeter Data-Gathering Devices, which are being developed by a group of volunteers at Civic Tech Toronto. The purpose of the device is to gather data about urban noise levels and upload that data to the cloud, whereupon the data can be made available to citizens, activists and lawmakers to help inform public policy around noise bylaws.

The first-generation device is based around an arduino microcontroller and a MEMS microphone module, which sends audio data over I2S protocol. The device is to be hosted by volunteers at their home or place of work, where it will stay, long-term, to gather noise data and upload it. Currently, the device requires USB power and a WiFi connection in order to operate. Future devices may differ in both respects.

We will try to maintain up-to-date documentation within each hardware/version folder so that volunteers can more easily help out with the project.

## Source Code

The [noisemeter-device](/noisemeter-device) folder contains the device's source code. The code can be built with the Arduino IDE or PlatformIO, and supports two ESP32 targets: a "breadboard" target for the early prototype, and a "pcb" target for the official circuit boards. See the source code's [README](/noisemeter-device/README.md) for build instructions. The source code is released under the [GNU GPL v3 license](/noisemeter-device/LICENSE).

## Hardware Files

The [hardware](/hardware) folder contains design files and documentation for each iteration of the noisemeter hardware. The PCBs are made using [KiCAD](https://www.kicad.org/) and are released under an [open hardware license](/hardware/pcb-rev2/LICENSE).

## Current Points of Contact for the project:

(May be subject to change)

- Gabe Sawnhey (Project Lead)
- Nick Barnard (Arduino)
- Clyne Sullivan (Arduino)
- Mitch Bechtel (API & Cloud DB)
