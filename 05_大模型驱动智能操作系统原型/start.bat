@echo off
setlocal
cd /d "%~dp0"

if not exist "config.json" (
    copy /Y "config.example.json" "config.json" >nul
    echo Created config.json from config.example.json.
    echo Please fill in your DeepSeek API key and WSL distro name, then run again.
    pause
    exit /b 1
)

where py >nul 2>nul
if %errorlevel%==0 (
    set "PY_CMD=py -3"
) else (
    where python >nul 2>nul
    if %errorlevel%==0 (
        set "PY_CMD=python"
    ) else (
        echo Python was not found in PATH.
        echo Install Python 3 and make sure "py" or "python" is available.
        pause
        exit /b 1
    )
)

echo Starting OS prototype service...
start "" http://127.0.0.1:8899
call %PY_CMD% app.py
set "APP_EXIT=%errorlevel%"

if not "%APP_EXIT%"=="0" (
    echo.
    echo Service failed to start. Check the error message above.
    pause
    exit /b %APP_EXIT%
)

endlocal
