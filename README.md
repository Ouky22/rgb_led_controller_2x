Programm that runs on Wi-Fi microcontroller (e.g. esp8266) to receive HTTP requests containing commands and to control conntected RGB-LED-Strips.
See https://github.com/Ouky22/MyRgb.git for an Android app that can send the commands.


# Protocol for commands

Commands have the following structure:

## ccc.abc.def.

- ccc = command identifier (which type of command) or value for red

## 0 <= ccc <= 255 -> change color

- ccc --> red value (0-255)
- abc --> green value (0-255)
- def --> blue value (0-255)

## 300.abc.xxx. - off command

- turn off certain LED strip
- abc --> id of strip

## 400.abc.xxx. - on command

- turn on certain LED strip
- abc --> id of strip

## 500.abc.xxx. - brightness command

- abc --> brightness value (0-255)

## 600.abc.xxx. - rgb show command

- abc --> speed (0-10)

## 700.xxx.xxx. - current settings command

- receive all current settings (e.g. color values, brightness, strips, which strip is enabled, status of rgb show)

## 800.xxx.xxx. - rgb alarm command

- trigger new rgb alarm
