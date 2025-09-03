@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Build + Burn helper for HV Trigger Async (Windows)
rem Requires: arduino-cli on PATH

where arduino-cli >nul 2>nul || (
  echo arduino-cli not found on PATH. Install from https://arduino.github.io/arduino-cli/latest/
  exit /b 1
)

set "SKETCH_DEFAULT=hv_trigger_async.ino"
set "FQBN_DEFAULT=esp32:esp32:esp32s3"
set "BUILD_DIR=build"

echo ==========================================================
echo Build + Burn (Windows)

for /f "tokens=1 delims=" %%A in ('arduino-cli core list ^| findstr /b esp32:esp32') do set CORE_FOUND=1
if not defined CORE_FOUND (
  echo Installing esp32 core ...
  arduino-cli core update-index
  arduino-cli core install esp32:esp32 || exit /b 1
)

set "SKETCH=%SKETCH_DEFAULT%"
set /p USEDEF=Use sketch %SKETCH_DEFAULT%? [Y/n] ^> 
if /I "%USEDEF%"=="n" (
  set /p SKETCH=Enter path to .ino: ^> 
)
if not exist "%SKETCH%" (
  echo Sketch not found: %SKETCH%
  exit /b 1
)

set "FQBN=%FQBN_DEFAULT%"
set /p TMP=Enter FQBN [%FQBN_DEFAULT%]: ^> 
if not "%TMP%"=="" set "FQBN=%TMP%"

if exist "%BUILD_DIR%" (
  set /p OW=Build folder '%BUILD_DIR%' exists. Overwrite/clean? [y/N] ^> 
  if /I not "%OW%"=="y" (
    echo Aborting per user choice.
    exit /b 1
  ) else (
    rmdir /s /q "%BUILD_DIR%"
  )
)

echo ==========================================================
echo Compiling: %SKETCH%
arduino-cli compile --fqbn "%FQBN%" --output-dir "%BUILD_DIR%" "%SKETCH%"
if errorlevel 1 (
  echo Compile failed. Retrying once...
  arduino-cli compile --fqbn "%FQBN%" --output-dir "%BUILD_DIR%" "%SKETCH%"
  if errorlevel 1 (
    echo Compile failed again.
    exit /b 1
  )
)
echo Build artifacts in: %BUILD_DIR%

set /p UP=Upload to device now? [Y/n] ^> 
if /I "%UP%"=="n" goto :done

echo Detecting serial ports...
set /a IDX=0
for /f "tokens=1,* delims= " %%P in ('arduino-cli board list ^| findstr /R "^COM[0-9]"') do (
  set /a IDX+=1
  set "PORT!IDX!=%%P"
  echo   !IDX!) %%P
)

if %IDX%==0 (
  echo No ports detected. Type a port (e.g., COM9):
  set /p PORT=^> 
  goto :upload
)

set /p CH=Select port [1-%IDX%]: ^> 
if "%CH%"=="" (
  echo Invalid selection
  exit /b 1
)
for /f "tokens=1 delims=" %%X in ("!PORT%CH%! ") do set "PORT=%%~X"

:upload
echo ==========================================================
echo Uploading to %PORT% ...
arduino-cli upload -p "%PORT%" --fqbn "%FQBN%" "%SKETCH%"
if errorlevel 1 (
  echo Upload failed. Retrying once...
  timeout /t 1 >nul
  arduino-cli upload -p "%PORT%" --fqbn "%FQBN%" "%SKETCH%"
  if errorlevel 1 (
    echo Upload failed again.
    exit /b 1
  )
)
echo Upload completed.

:done
echo ==========================================================
echo Done.
exit /b 0

