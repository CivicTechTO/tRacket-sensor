# Project Noisemeter Device

Civic Tech TO

Current release candidate: /src/esp32c3/v1

## Overview

This repo contains all source code for the Project Noisemeter Data-Gathering Devices, which are being developed by a group of volunteers at Civic Tech Toronto. The purpose of the device is to gather data about urban noise levels and upload that data to the cloud, whereupon the data can be made available to citizens, activists and lawmakers to help inform public policy around noise bylaws.

The first-generation device is based around an arduino microcontroller and a MEMS microphone module, which sends audio data over I2S protocol. The device is to be hosted by volunteers at their home or place of work, where it will stay, long-term, to gather noise data and upload it. Currently, the device requires USB power and a WiFi connection in order to operate. Future devices may differ in both respects.

## Source Code

The [src](/src) folder contains source code for all current and past device iterations. The code is organised by device type (eg Raspberry Pi, ESP8266, ESP32). Within those folders, versioning folders can be found (eg v1, v2, etc). We will try to maintain up-to-date documentation within each hardware/version folder so that volunteers can more easily help out with the project.

Many source code repos will be missing some files, which have been gitignored to keep secret information (API keys, WiFi credentials, etc) out of this public repo. Running much of the code will involve creating/generating these missing files yourself. Repos should have instructions in their readme files. If the code won't compile, there's a good chance you will be missing a secret file or Arduino library. Check error output for clues.

## Current Points of Contact for the project:

(May be subject to change)

- Gabe Sawnhey (Project Lead)
- Nick Barnard (Arduino)
- Clyne Sullivan (Arduino)
- Mitch Bechtel (API & Cloud DB)
