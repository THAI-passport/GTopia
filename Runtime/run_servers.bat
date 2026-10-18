@echo off

if /i "%~1"=="--keepalive" goto keepalive

title GTopia Launcher
cd /d "%~dp0"

set GAME_SERVER_COUNT=0
if exist servers.txt (
    for /f "tokens=4 delims=|" %%a in ('findstr /r "^add_server|" servers.txt') do (
        set /a GAME_SERVER_COUNT+=%%a
    )
)

if %GAME_SERVER_COUNT% equ 0 set GAME_SERVER_COUNT=1
echo Game servers to start: %GAME_SERVER_COUNT%

if not exist Master.exe (
    echo Master.exe not found. Did you run the compile script?
    pause
    exit /b 1
)

if not exist GameServer.exe (
    echo GameServer.exe not found. Did you run the compile script?
    pause
    exit /b 1
)

echo Building HTTP server...
pushd ..\HTTPServer
go build -o httpserver.exe main.go
if errorlevel 1 (
    popd
    echo HTTP server build failed. Is Go installed?
    pause
    exit /b 1
)
popd

echo Starting HTTPS Server...
start "GTopia - HTTPS Server" /d "%~dp0..\HTTPServer" cmd /c ""%~f0" --keepalive httpserver.exe"
timeout /t 2 /nobreak >nul

echo Starting Master Server...
start "GTopia - Master Server" cmd /c ""%~f0" --keepalive Master.exe"
timeout /t 3 /nobreak >nul

for /L %%i in (1,1,%GAME_SERVER_COUNT%) do (
    echo Starting Game Server %%i...
    start "GTopia - Game Server %%i" cmd /c ""%~f0" --keepalive GameServer.exe --id %%i"
    timeout /t 1 /nobreak >nul
)

echo ==================================================
echo All servers started. Each one runs in its own window
echo and restarts by itself if it crashes.
echo To stop everything, run stop_servers.bat
pause
exit /b 0

:keepalive
shift
:loop
%1 %2 %3 %4 %5
echo.
echo [%date% %time%] Server stopped. Restarting in 5 seconds...
echo (run stop_servers.bat to stop for good)
timeout /t 5 /nobreak >nul
goto loop