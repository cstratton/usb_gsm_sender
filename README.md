# usb_gsm_sender
Report data from embedded devices by hosting a cheap USB GSM modem (E303, etc)

Economies of scale often mean that the USB-dongle version of some useful peripheral is substantially cheaper than an SPI, I2C, or UART interfaced version handy for use in embedded projects.  There has been some progress on wifi with the esp8266, but other things like GSM retain a large cost difference between "consumer" USB and "embedded" versions.  Today many current ARM Cortex-M4 and even M0 processors are now capable of acting as embedded USB hosts.  Since these are often available on cheaper ($10-12) eval boards than the Arduino-type boards many habitually reach for when prototyping simple embedded projects, they provide both a great opportunity to use cheap GSM hardware in the form of a USB dongle, and a better host for the actual application program which needs to scan sensors, control actuators, and send messages over GSM home to your server.

For more information, see the Wiki at

https://github.com/cstratton/usb_gsm_sender/wiki

Where's the code you ask?   Still needing some review before posting.   But I'm going to go ahead and create the repository right now in order to use the wiki for project notes and lessons learned.
