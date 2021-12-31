# Picoclick-C3

The Picoclick C3 as well as its little brother (the C3T, T = tiny) are the successors of the well known [Picoclick](https://github.com/makermoekoe/Picoclick). The name is related to its new processor: the [ESP32-C3](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf) single core RISC-V 160MHz CPU. It enables tons of new features, but especially for the IoT button use case Bluetooth Low Energy (BLE) could be the most interesting one.

As the C3 is in development at the moment, most of the files are related to the C3T so far.

Here are some specs of the Picoclick C3 and the C3T:
- Dimensions are 18.0×20.0mm for the C3 and 10.5x18.0mm for the C3T
- Ultra low stand-by current due to latching circuit (no deep sleep)

## GPIOs


Function | GPIO C3T | GPIO C3 | Mode
-------- | -------- | -------- | --------
WS2812 Din | GPIO6 | GPIOX | Output
Latch* | GPIO3 | GPIOX | Output
Button | GPIO5 | GPIOX | Input
Charge Stat. | GPIO1  | GPIOX | Input
Bat Voltage | GPIO4 | GPIOX | Input
