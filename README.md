# Picoclick-C3

The Picoclick C3 as well as its little brother (the C3T, T = tiny) are the successors of the well known [Picoclick](https://github.com/makermoekoe/Picoclick). The name is related to its new processor: the [ESP32-C3](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf) single core RISC-V 160MHz CPU.

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


## Speed up boot process

Due to the latching circuit, the button press has to be longer than the boot up time of the processor, because the first task should have been executed once the button is released. For most of the use cases of the Picoclick the first task is to toggle the latch GPIO high in order to enable power hold feature of the Picoclick. If this task haven't been executed before the button is released, the Picoclick is going to standby mode without finishing its main task.
Standard boot up time of the ESP32-C3 is around 300ms. So the first task will be executed after these 300ms. This is far too long because a standard button press (especially with the metal buttons on the C3T) is around 60ms-150ms. Almost every task of the Picoclick would have been interrupted in this case (unless you press and hold the button...).
To speed up the boot process I got familiar with the menuconfig of the ESP-IDF. Actually the reason why I've switched from the Arduino framework to ESP-IDF was that I get access to all the configs of the ESP32-C3.

Things I have done so far:
- Set *Flash SPI mode* from *DIO* to *QIO* (in Serial flasher config)
- Set *Log output* from *Info* to *Warning* (in component config)
- Set *Bootloader log verbosity* from *Info* to *Warning* (in Bootloader config)
- Enable *Skip image validation from power on* (in Bootloader config)

These points result in a boot up time of around 69ms which is almost quite fantastic. The test I've done so far were quite sufficient.
