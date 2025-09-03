@echo off
REM Thin wrapper to launch the PowerShell build script
setlocal
set SCRIPT=%~dp0buildAndBurn.ps1
if not exist "%SCRIPT%" (
  echo Cannot find PowerShell script: %SCRIPT%
  exit /b 1
)
powershell -ExecutionPolicy Bypass -NoProfile -File "%SCRIPT%"
exit /b %ERRORLEVEL%
