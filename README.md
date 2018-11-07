# MM-control-01
MMU 3-axis stepper control

## Building
### Cmake
You need cmake, avr-gcc, avr-libc and cmake supported build system (e.g. ninja) installed.

Out of source tree build is recommended, in case of Eclipse CDT project file generation is necceessary. If you don't want out of source tree build, you can skip this step.
~~~
cd ..
mkdir MM-control-01_cmake
cd MM-control-01_cmake
~~~
Generate build system - consult cmake --help for build systems generators and IDE project files supported on your platform.
~~~
cmake -G "build system generator" path_to_source
~~~
example 1 - build system only
~~~
cmake -G "Ninja" ../MM-control-01
~~~
example 2 - build system and project file for your IDE
~~~
cmake -G "Eclipse CDT4 - Ninja ../MM-control-01
~~~
Invoke build system you selected in previous step. Example:
~~~
ninja
~~~
file MM-control-01.hex is generated.

### Arduino
Recomended version is arduino 1.8.5.  
in MM-control-01 subfolder create file version.h  
use version.h.in as template, replace ${value} with numbers or strings according to comments in template file.  
create file dirty.h with content if you are building unmodified git commit
~~~
#define FW_LOCAL_CHANGES 0
~~~
or
~~~
#define FW_LOCAL_CHANGES 1
~~~
if you have uncommitted local changes.
#### Adding MMUv2 board
In Arduino IDE open File / Settings  
Set Additional boards manager URL to:  
https://raw.githubusercontent.com/prusa3d/Arduino_Boards/master/IDE_Board_Manager/package_prusa3d_index.json  
Open Tools / Board: / Boards manager...
Install Prusa Research AVR Boards by Prusa Research  
which contains only one board:  
Original Prusa i3 MK3 Multi Material 2.0

Select board Original Prusa i3 MK3 Multi Material 2.0

Bootloader binary is shipped with the board, source is located at https://github.com/prusa3d/caterina
#### Build
click verify to build
### PlatformIO
PlatformIO build is not supported by Prusa Research, please report any PlatformIO related issues at https://github.com/awigen/MM-control-01/issues

## Flashing
### Windows
#### Arduino
click Upload
#### Slic3er
Hex file needs to be edited to be recognized as for MMUv2, to be specified later (in several years)
#### Avrdude
Board needs to be reset to bootloader. Bootloader has 5 seconds timeout and then returns to the application.

This can be accomplished manually by clicking reset button on MMU, or programmatically by opening and closing its virtual serial line at baudrate 1500.

Than flash it using following command, replace \<virtual serial port\> with CDC device created by MMU usually com\<nr.\> under Windows and /dev/ttyACM\<nr.\> under Linux. -b baud rate is don't care value, probably doesn't have to be specified at all, as there is no physical uart.
~~~
avrdude -v -p atmega32u4 -c avr109 -P <virtual serial port> -b 57600 -D -U flash:w:MM-control-01.ino.hex:i
~~~

### Linux
Same as Windows, but there is known issue with ModemManager:

If you have the modemmanager installed, you either need to deinstall it, or blacklist the Prusa Research USB devices:

~~~
/etc/udev/rules.d/99-mm.rules

# Original Prusa i3 MK3 Multi Material 2.0 upgrade
ATTRS{idVendor}=="2c99", ATTRS{idProduct}=="0003", ENV{ID_MM_DEVICE_IGNORE}="1"
ATTRS{idVendor}=="2c99", ATTRS{idProduct}=="0004", ENV{ID_MM_DEVICE_IGNORE}="1"

$ sudo udevadm control --reload-rules
~~~
A request has been sent to Ubuntu, Debian and ModemManager to blacklist the whole Prusa Research VID space.

https://bugs.launchpad.net/ubuntu/+source/modemmanager/+bug/1781975

https://bugs.debian.org/cgi-bin/pkgreport.cgi?dist=unstable;package=modemmanager

and reported to
https://lists.freedesktop.org/archives/modemmanager-devel/2018-July/006471.html

## Building documentation
Run doxygen in MM-control-01 folder.
Documentation is generated in Doc subfolder.
