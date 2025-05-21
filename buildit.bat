@echo off
setlocal enabledelayedexpansion

:: --- Get current date and time ---
:: For date, assuming a DD.MM.YYYY or similar format with . or / delimiters
:: and that DD, MM, YY will be the first three tokens
for /f "tokens=1-3 delims=/.-" %%a in ('echo %date%') do (
    set "DD=%%a"
    set "MM=%%b"
    set "YY=%%c"
)
:: Handle potential single-digit day/month/year (if your locale gives them)
if "%DD:~0,1%" equ " " set "DD=0%DD:~1%"
if "%MM:~0,1%" equ " " set "MM=0%MM:~1%"
if "%YY:~0,1%" equ " " set "YY=0%YY:~1%" :: Just in case year is like '5' instead of '05'

:: For time, we need to be more careful with AM/PM.
:: We'll take all tokens, then ignore the last one if it's AM/PM.
:: Assuming time format like HH:MM:SS.xx or HH:MM:SS AM/PM
for /f "tokens=1-4 delims=:." %%a in ('echo %time%') do (
    set "HH=%%a"
    set "MI=%%b"
    set "SS=%%c"
    set "AMPM=%%d" :: This will capture 'PM', 'AM', or remaining milliseconds
)

:: Now, we need to re-extract SS if AMPM was actually milliseconds, not AM/PM.
:: If AMPM contains letters (A or P), it's the AM/PM indicator, so SS is %%c.
:: If AMPM contains only numbers, it's milliseconds, so SS needs to include it.
:: A simpler approach is to just take the first 3 tokens and hope the SS token doesn't include milliseconds,
:: or if it does, it's just numbers. The AMPM removal is crucial if it appears.

:: Let's re-think the time parsing for robustness against AM/PM
:: We can use 'findstr' to strip the AM/PM part if it exists.
:: This is a bit more complex, but safer.

set "current_time_raw=%time%"
:: Remove AM/PM if present (case-insensitive)
if /i "%current_time_raw:AM=%" NEQ "%current_time_raw%" set "current_time_raw=%current_time_raw:AM=%"
if /i "%current_time_raw:PM=%" NEQ "%current_time_raw%" set "current_time_raw=%current_time_raw:PM=%"
:: Trim leading/trailing spaces that might be left
for /f "tokens=*" %%x in ("%current_time_raw%") do set "current_time_raw=%%x"

:: Now parse the cleaned time string (e.g., "11:14:24.56" or "11:14:24")
for /f "tokens=1-3 delims=:." %%a in ('echo %current_time_raw%') do (
    set "HH=%%a"
    set "MI=%%b"
    set "SS=%%c"
)

:: Pad single-digit hour, minute, and second with a leading zero
if "%HH:~0,1%" equ " " set "HH=0%HH:~1%"
if "%MI:~0,1%" equ " " set "MI=0%MI:~1%"
if "%SS:~0,1%" equ " " set "SS=0%SS:~1%"

set "filename=build-%DD%.%MM%.%YY%-%HH%:%MI%"

echo "%filename%"

if "%1" == "server" (
    echo Building server...
    .\ACWServer\make.bat
    echo Packaging server...
    copy ".\libraries\" ".\builds\server\%filename%\"
    move ".\ACWServer\ACW.exe" ".\builds\server\%filename%\ACW-server.exe"
    echo Done! Sucessful build and package!
)
if "%1" == "client" (
    echo Building client...
    .\ACWClient\make.bat
    echo Packaging client...
    copy ".\libraries\" ".\builds\client\%filename%\"
    move ".\ACWClient\ACW.exe" ".\builds\client\%filename%\ACW-client.exe"
    echo Done! Sucessful build and package!
)

pause
