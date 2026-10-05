@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
set "CN_RESULT=%ERRORLEVEL%"
if not "%CN_RESULT%"=="0" echo Build failed with exit code %CN_RESULT%.
exit /b %CN_RESULT%
