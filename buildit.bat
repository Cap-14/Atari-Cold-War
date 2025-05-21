:: some time type shiiii
@echo off
for /f "tokens=1-6 delims=/: " %%a in ('robocopy . . /njh ^| find ""') do (
    set "DD=%%a"
    set "MM=%%b"
    set "YY=%%c"
    set "HH=%%d"
    set "MI=%%e"
    set "SS=%%f"
)

:: Pad single-digit day, month, hour, minute, and second with a leading zero if necessary
if "%DD:~0,1%" equ " " set "DD=0%DD:~1%"
if "%MM:~0,1%" equ " " set "MM=0%MM:~1%"
if "%HH:~0,1%" equ " " set "HH=0%HH:~1%"
if "%MI:~0,1%" set "MI=0%MI:~1%"
if "%SS:~0,1%" set "SS=0%SS:~1%"

set "filename=build-%DD%%MM%%YY%-%HH%%MI%%SS%.txt"

if ["%~1" == "server"] (
    echo Building server...
    .\ACWServer\make.bat
    echo Packaging server...
    copy .\libraries\ .\builds\server\%filename%\
    move .\ACWServer\ACW.exe .\builds\server\%filename%\ACW-server.exe
    echo Done! Sucessful build and package!
)
else if ["%~1" == "client"] (
    echo Building client...
    .\ACWClient\make.bat
    echo Packaging client...
    copy .\libraries\ .\builds\server\%filename%\
    move .\ACWClient\ACW.exe .\builds\client\%filename%\ACW-client.exe
    echo Done! Sucessful build and package!
)

pause
