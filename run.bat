@echo off
setlocal
cd /d "%~dp0"

echo Building backend...
cmake -S backend -B backend\build
if errorlevel 1 goto :error
cmake --build backend\build --config Release
if errorlevel 1 goto :error

echo Building frontend...
dotnet build frontend\WeatherApp.csproj
if errorlevel 1 goto :error

echo Starting YourWeather...
dotnet run --project frontend\WeatherApp.csproj
goto :end

:error
echo Build failed.
exit /b 1

:end
endlocal
