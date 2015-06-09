# usb_gsm_sender
Report data from embedded devices by hosting a cheap USB GSM modem (E303, etc)

Economies of scale often mean that the USB-dongle version of some useful peripheral is substantially cheaper than an SPI, I2C, or UART interfaced version handy for use in embedded projects.  There has been some progress on wifi with the esp8266, but other things like GSM retain a large cost difference between "consumer" USB and "embedded" versions.  Today many current ARM Cortex-M4 and even M0 processors are now capable of acting as embedded USB hosts.  Since these are often available on cheaper ($10-12) eval boards than the Arduino-type boards many habitually reach for when prototyping simple embedded projects, they provide both a great opportunity to use cheap GSM hardware in the form of a USB dongle, and a better host for the actual application program which needs to scan sensors, control actuators, and send messages over GSM home to your server.

For more information, see the Wiki at

https://github.com/cstratton/usb_gsm_sender/wiki

Some very, very alpha code has now been added, which uses an mbed-derived source base and a hand-modified Makefile which attempts to provide some flexibility across STM32F and KL25Z targets (K20's should have worked, but oddly don't).

License: Good question!   Much of this depends on mbed-derived code much of which is apache licensed.  However, some pieces may be under other licenses.  As for the project overall, while I plan to make it usefull open, until I've completed a license review of all the pieces, the simple answer is that for the original parts or original aspects of derivative parts, as of the present NO LICENSE IS OFFERED, which is to say that copyright law is in full force, and no authorization to do anything which copyright law would prohibit is yet being offered.
