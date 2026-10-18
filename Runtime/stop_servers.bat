@echo off
echo Stopping GTopia...

taskkill /fi "WINDOWTITLE eq GTopia - *" /t /f >nul 2>&1

taskkill /im GameServer.exe /f >nul 2>&1
taskkill /im Master.exe /f >nul 2>&1
taskkill /im httpserver.exe /f >nul 2>&1

echo Done.
pause