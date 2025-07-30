@echo off
@echo off
echo 11:32:19: Running steps for project SerialPrograms...
echo 11:32:19: Starting: "C:\Qt6.8.3\Tools\CMake_64\bin\cmake.exe" --build C:/Users/w/dev/PokemonAutomation/Arduino-Source/build-SerialPrograms-Desktop_Qt_6_8_3_MSVC2022_64bit-RelWithDebInfo --target all

REM Set up Visual Studio environment like Qt Creator does
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

C:\Qt6.8.3\Tools\CMake_64\bin\cmake.exe --build build-SerialPrograms-Desktop_Qt_6_8_3_MSVC2022_64bit-RelWithDebInfo --target all
set EXIT_CODE=%ERRORLEVEL%

echo.
echo ========================================
echo Build completed with exit code: %EXIT_CODE%
echo ========================================