# tRacket: Community-Driven Urban Noise Monitoring

This repository contains all source code and hardware design files for the [tRacket](https://tracket.info/) noise sensor.

## Overview

Environmental noise, especially in urban settings, is a [known public health concern](https://www.toronto.ca/wp-content/uploads/2017/11/8f98-tph-How-Loud-is-Too-Loud-Health-Impacts-Environmental-Noise.pdf):

> The growing body of evidence indicates that exposure to excessive environmental
noise does not only impact quality of life and cause hearing loss but also has other health impacts, such as cardiovascular effects, cognitive impacts, sleep disturbance and mental health effects.

The tRacket noise sensor continuous monitors ambient sound levels (in [decibels](https://en.wikipedia.org/wiki/Decibel), dBA) and sends these measurements to an [online dashboard](https://dashboard.tracket.info/locations) ([also on GitHub](https://github.com/CivicTechTO/tRacket-dashboard)). Our goal is to help local communities better understand the sound levels and extreme noise events that they experience day to day.

The project has been started and is maintained by volunteers from the [CivicTech Toronto](http://www.civictech.ca) community.

## Privacy

We followed Privacy by Design principles in setting up the data collection. 

1. The sound meter devices are deployed on private properties in residential areas at different locations in the city. We are **not publishing exact device locations**. 
2. The devices **do not record sound** only sound levels in A-weighted decibel levels (dBA)(https://en.wikipedia.org/wiki/Decibel). 
3. We calculate minimum and maximum sound levels at 5 minute intervals on the device and **only broadcast these aggregate values** (along wiht the device ID) to a database.

## Source Code

The [noisemeter-device](/noisemeter-device) folder contains the device's source code, which is written in C++ and can be built using [PlatformIO](https://platformio.org/). Build instructions are available [in the online documentation](https://civictechto.github.io/tRacket-sensor/md_noisemeter_device_BUILD.html).

The source code is released under the [GNU GPL v3 license](/noisemeter-device/LICENSE).

## Hardware Files

The [hardware](/hardware) folder contains design files and documentation for each iteration of the sensor hardware. The PCBs are designed using [KiCAD](https://www.kicad.org/).

The hardware design files are released under an [open hardware license](/hardware/pcb-rev2/LICENSE).

## For Developers

* Check out the [Issues page](https://github.com/CivicTechTO/tRacket-sensor/issues) to see current bugs and feature requests. Any contributions are welcome!
* Online documentation of the firmware [is available here](https://civictechto.github.io/tRacket-sensor/).

