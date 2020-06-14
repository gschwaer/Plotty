# Simple Plotter

This Arduino based G-code plotter is inspired by the [BrachioGraph](https://github.com/evildmp/BrachioGraph). The goal is to create a simple, cheap and easy do build plotter. A classic rainy weekend project.

## Why not Brachiograph?

I build the BrachioGraph hardware first and tried to use the BrachioGraph software. But it didn't show the best results with the Raspberry Pi Zero I had. Part of which might be the missing native PWM support of the Raspberry Pi. So I decided to replace the processing unit with an Arduino Nano Every.

## Requirements

Everything the [BrachioGraph](https://github.com/evildmp/BrachioGraph) requires, minus a Raspberry Pi, plus an Arduino Nano Every (probably other Arduinos work as well) and a fineliner (I used a Stabilo point 88 fine 0.4).

__Warning__: Don't use cheap servos! I tried to use cheap servos at first (seemingly a fake of [SG90](http://www.towerpro.com.tw/product/sg90-7/) without the Tower Pro label) and the results were terrible (I still use one for the pen lifting servo, since no precision is required there). Now I am using Modelcraft MC-1811 and they work ok.

## Setup

For the hardware follow the instructions on the [BrachioGraph](https://github.com/evildmp/BrachioGraph) project. It helped to add a counter weight to the "upper arm" to allow the pen to glide more freely.

The software is a classic Arduino project:
```
Sketch uses 9319 bytes (18%) of program storage space. Maximum is 49152 bytes.
Global variables use 258 bytes (4%) of dynamic memory, leaving 5886 bytes for local variables. Maximum is 6144 bytes.
```

## How it works

The plotter plots G-code (only some commands are supported, see source code). I used the tool [jscut](http://jscut.org/jscut.html) to generate G-code from svg images and all the G-code it generated is supported.

The lifting of the pen works as follows:
* G0/G1 movement with Z >= 0: pen switches to the up position
* G0/G1 movement with Z < 0: pen switches to the down position

The coordinate system is classic for G-code:
* X increases to the right
* Y increases upwards

To "talk" to the Arduino I use the Arduino's serial connection. The bash script `plot.sh path/to/file.gcode` sends all the G-code commands in a file to the Arduino (adjust the serial port before using it).

## Examples

There are two example G-codes in the `examples` folder (both 70 mm x 70 mm, USB stick as size reference):

* Hello Globe (svg from [this side](https://www.svgrepo.com/svg/150084/earth-globe))

![hello_globe_result](https://user-images.githubusercontent.com/3410079/84601046-dc656900-ae7d-11ea-967c-2702b8fbd78f.jpeg)

* Derp (png from [this side](https://www.pngwing.com/en/free-png-ztiui) converted to svg using [this side](https://bild.online-convert.com/de/umwandeln-in-avg))

![derp_result](https://user-images.githubusercontent.com/3410079/84601062-f606b080-ae7d-11ea-9124-554f4559fc1f.jpeg)

(The paper started to warp a little because of the wet ink, which caused the line between the eyes.)

## Plot your own images

An example on how you can create your own G-code, can be found [here](https://www.sourcerabbit.com/Help/tutorial-i-164-t-how-to-generate-gcode-from-an-svg-file-using-jscut.htm).

My settings for jscut can be found in the file `settings.jscut`. Make sure to click on "Zero lower left" before exporting the G-code.

The Plotter only supports absolute coordinates in mm.
