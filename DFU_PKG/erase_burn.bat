@echo off
echo Starting J-Link Commander to format the chip...

:: Define the J-Link Commander executable path if necessary
set JLINK_PATH="C:\Program Files\SEGGER\JLink\"

:: Run J-Link Commander with commands to connect and format the chip
%JLINK_PATH%\JLink.exe -CommandFile format_chip.jlink

echo Chip formatting complete.
pause