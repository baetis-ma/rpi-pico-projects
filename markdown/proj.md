#### Here is a bundle of RPi pico projects for anyone interested. They are all done using the PICO SDK using the C (or C++) language.

#### The SDK is pretty easy to install on major platforms - linux, windows, mac - well documented walk throughs are available on web for each. The components are readily available and inexpensive. I have loaded the project into github - [GitHub - baetis-ma/rpi-pico-projects](https://github.com/baetis-ma/rpi-pico-projects/tree/main) , the following bullets contain outlines of the individual projects. All you need to tun these projects are the pico board, a peripheral board and a usb cable to your computer.

## Heart Rate Monitor

#### Core1 sets up the MAX30102 configuration, then continuously reads the 16 bit reflection return of the red and infrared leds into the red and infrared photosenors into a circular buffer shared with Core0 via the interprocessor buffer, an end pointer is also supplied. Core1 treats the data to a chebyshev 4 pole band pass 7 to 0.7 hertz 100 samples/sec filter, and extracts the pulse rate and calculates the spO2 concentration from the ir vs red ratios.

<img title="" src="https://github.com/baetis-ma/rpi-pico-projects/blob/main/markdown/heart.png?raw=true" alt="" height="" width="424" data-align="center">

#### Core0 simply presents the data streaming in from core1. In the case of a pico-w, a webpage can be presented with the data updating about once a second (tcp and browsers are slow) with a browser at \<ip address\>/index.html. Alternatively, in the absence of the wifi radio, the raw data serial output can be piped into a perl program and gnuplotted, with updates at 10fps as in the accompanying picture, data represents the last five seconds of collected data.

## Audio Spectrum Waterfall Plot

#### This program reads the i2s microphone output (inmp441 or msm261s4030h0) at a 22.05KHz sample rate in bursts of 2048 samples and performs an fft on this data to produce a 0-11KHz spectrum every 100msec. The spectrum is output to the serial port and piped into a perl program handling a circular buffer array which is then gnuplotted on a color bar (x-freq,y-time and z color amplitude) and updated 10fps, the graph shows the last 50 seconds of time. The attached picture was taken in a room with a few birds doing accompaniment and a 20-20KHz swept tone with a not so great speakers.

<img title="" src="https://github.com/baetis-ma/rpi-pico-projects/blob/main/markdown/spectrum.png?raw=true" alt="" height="" width="272" data-align="center">

## Ten Axis Accelerometer-Gyroscope-Magnetometer-Pressure with OLED

#### Core1 is continuously reading the MPU6050 6 axis accelerometer and gyroscopes at 2KHz. The results are fusion filtered to produce pitch and roll values that are less noisey and non-drifting than those that can be obtained by the accelerometers or gyroscopes individually, the results are values that seem to be accurate to better than a degree and do not drift even after hours.

#### Core0 gets data from the magnetometer (qmc5883l) and the pressure sensor (bmp280) and presents screen summary to the oled display (ssd1306). The elevation is calculated from change in atmospheric pressure since calibration and on some days can drift pretty quickly.

<img title="" src="https://github.com/baetis-ma/rpi-pico-projects/blob/main/markdown/imu.jpg?raw=true" alt="" width="188" data-align="center">

## Text scroller

#### This one is my favorite, I have 'A River Runs Through It' stored in the flash memory, it scrolls through about every 12 hours and I have it hanging in my living room. Each time I walk by I try to remember where in the book it currently is. The 16MB flash version of the pico board could store about a dozen books, any book that you have in .mobi .epub .pdf, etc can be converted with ebook-convert book.mobi book.txt --asciize (the asciize converts unicode). `const uint8_t __in_flash() text[] = "insert string here"` stores the book text in flash instead taking up the limited sram. The double quotes need to be block replaced with single quotes to quote the string.

#### The project reads text characters one at a time from the book string on flash, uses the char indexed to an address offset to read a font array.

#### The font is then decoded into led being on or off and the result interlaced along the 8x64 length of the array, the font is proportionally spaced for narrow characers. The serial scan advances by one column every 30ms for what, I find, a pretty good reading speed. The font came from a standard 5x7 font table that was found on the web and modified to 5x8 to lower extend the gjpqy,;s. Finding a good diffuser is a challenge.

![](https://github.com/baetis-ma/rpi-pico-projects/blob/main/markdown/scroll.jpg?raw=true)

#### This project requires a (couple) ws2812b 8x32 led arrays (you can get these for about $6 each).

## Logic Analyzer

#### Captures the state of the 32 RP2040 gpios (inputs and outputs) and provides a graphical interface updating up to 50fps. Data can be captured up to the np2040 clock speed (125MHz). The sample rate, update rate, number of samples (up to 50K), trigger pin and level as well as handling labels and busses are configurable with the same serial port used to program the device. The serial output is piped to a perl program that formats data and updates gnuplot.

#### The logic analyzer runs completely within core 1 of the processor so will not interfere with single processor applications running in core0.

<img title="" src="https://github.com/baetis-ma/rpi-pico-projects/blob/main/markdown/la.png?raw=true" alt="" width="454" data-align="center">

#### Shell scripts to automate routine procedures

##### I don't bother with an IDE and do most of my work with terminals and command lines. I wrote a couple bash scripts to automate routine tasks-

##### . ./setenv.sh - must be sourced (precede executable with ',' or 'source'), after that from project base directory - mkdir build; cd build; cmake ..; make; then

##### ./program.sh -- program 'magic baud' to avoid having to do the reset-boot button thing, then loads program to flash and starts program

##### ./flash.sh - same as above, but takes monitor or minicom argument to start monitoring serial output or launching minicom for bidirectional serial communication with RP2040.

##### ./monitor.sh -- just starts looking at the serial output on the terminal.

##### Notes: my system uses /dev/ttyACM0 as the serial port connected to the RP2040, you my need to edit \*.sh to the serial port that your system connects to the RP2040. On my linux platform I added /dev/ttyACM0 to the dialout group to avoid permission errors.

##### If the rp2040 has crashed or is otherwise not running the stdio\_init\_all() you will have to reset the rp2040 while holding the boot button to restart the device in boot mode.
