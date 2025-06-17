@echo off
setlocal enabledelayedexpansion

for /f "tokens=1-3 delims=/.-" %%a in ('echo %date%') do (
    set "DD=%%a"
    set "MM=%%b"
    set "YY=%%c"
)

if "%DD:~0,1%" equ " " set "DD=0%DD:~1%"
if "%MM:~0,1%" equ " " set "MM=0%MM:~1%"
if "%YY:~0,1%" equ " " set "YY=0%YY:~1%" :: Just in case year is like '5' instead of '05'

for /f "tokens=1-4 delims=:." %%a in ('echo %time%') do (
    set "HH=%%a"
    set "MI=%%b"
    set "SS=%%c"
    set "AMPM=%%d" :: This will capture 'PM', 'AM', or remaining milliseconds
)

set "current_time_raw=%time%"

if /i "%current_time_raw:AM=%" NEQ "%current_time_raw%" set "current_time_raw=%current_time_raw:AM=%"
if /i "%current_time_raw:PM=%" NEQ "%current_time_raw%" set "current_time_raw=%current_time_raw:PM=%"

for /f "tokens=*" %%x in ("%current_time_raw%") do set "current_time_raw=%%x"

for /f "tokens=1-3 delims=:." %%a in ('echo %current_time_raw%') do (
    set "HH=%%a"
    set "MI=%%b"
    set "SS=%%c"
)

if "%HH:~0,1%" equ " " set "HH=0%HH:~1%"
if "%MI:~0,1%" equ " " set "MI=0%MI:~1%"
if "%SS:~0,1%" equ " " set "SS=0%SS:~1%"

set "filename=build-%DD%.%MM%.%YY%-%HH%.%MI%.%SS%"

echo Producing build: %filename%

if "%1" == "server" (
    echo Building server...
    cd ACWServer
    .\make.bat
    echo %CD%
    echo Packaging server...
    mkdir ".\builds\server\%filename%"
    copy ".\libraries\" ".\builds\server\%filename%\"
    move ".\ACWServer\ACW.exe" ".\builds\server\%filename%\ACW-server.exe"
    echo Done! Sucessful build and package!
)
if "%1" == "client" (
    echo Building client...
    cd ACWClient
    .\make.bat
    echo Packaging client...
    mkdir ".\builds\client\%filename%"
	mkdir ".\builds\client\%filename%\Audio\"
	mkdir ".\builds\client\%filename%\Data\"
	mkdir ".\builds\client\%filename%\Graphics\"
	copy ".\ACWClient\Audio\" ".\builds\client\%filename%\Audio\"
	copy ".\ACWClient\Data\" ".\builds\client\%filename%\Data\"
	copy ".\ACWClient\Graphics\" ".\builds\client\%filename%\Graphics\"
    copy ".\libraries\" ".\builds\client\%filename%\"
    move ".\ACWClient\ACW.exe" ".\builds\client\%filename%\ACW-client.exe"
    echo Done! Sucessful build and package!
)
