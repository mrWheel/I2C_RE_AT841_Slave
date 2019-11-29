#!/bin/bash
#-----------------------------------------------------
# command to flash I2C_RE_Tine841_Slave firmware to
# a ATTiny841 chip
#-----------------------------------------------------
#
#-->USER=<some user>
#
if [ -z ${USER} ]
then
	echo "First set USER in line 7 !!!"
	exit 1
fi
#
PATH2PROJECTDIR=/Users/${USER}/Documents/ArduinoProjects
#
PATH2AVRDUDE=/Users/${USER}/Library/Arduino15/packages/arduino/tools/avrdude/6.3.0-arduino17/bin/avrdude
PATH2CONF=/Users/${USER}/Library/Arduino15/packages/ATTinyCore/hardware/avr/1.3.2/avrdude.conf
PATH2OPTIBOOT=/Users/${USER}/Library/Arduino15/packages/ATTinyCore/hardware/avr/1.3.2/bootloaders/optiboot/optiboot_attiny84_8000000L.hex
PATH2HEX=${PATH2PROJECTDIR}/I2C_RotaryEncoder/I2C_RE_Tiny841_Slave/I2C_RE_Tiny841_Slave.ino.hex
#
PROGRAMMER=USBasp
FUSESETTINGS=" -Uefuse:w:0xFE:m -Uhfuse:w:0b11010111:m -Ulfuse:w:0xE2:m "
#
#---set fuses--
echo "set fuses .."
${PATH2AVRDUDE} -C${PATH2CONF} -v -pattiny841 -c${PROGRAMMER} -e ${FUSESETTINGS}
EXITCODE_F=${?}
echo "ExitCode setting fuses [${EXITCODE_F}] .."
#
#---flash bootloader (optiboot) ----
echo "flash bootloader (optiboot) .."
${PATH2AVRDUDE} -C${PATH2CONF} -v -pattiny841 -c${PROGRAMMER} -Uflash:w:${PATH2OPTIBOOT}:i
EXITCODE_B=${?}
echo "ExitCode flashing bootloader [${EXITCODE_B}] .."
#
#--flash firmware-------------------
echo "flash firmware .."
${PATH2AVRDUDE} -C${PATH2CONF}  -pattiny841 -c${PROGRAMMER} -Uflash:w:${PATH2HEX}:i
EXITCODE_H=${?}
echo "ExitCode flashing firmware [${EXITCODE_H}] .."
#
#if [ ${EXITCODE_F} -eq 0 ] && [ ${EXITCODE_B} -eq 0 ] && [ ${EXITCODE_H} -eq 0 ]
#then
	#echo "Done with no errors!"
#else
	#echo "ERRORS!!!!"
#fi
#
