@echo off
REM Compila Lowpass Visajoso (VST3) en Windows.
REM Requiere Visual Studio 2022 (C++), CMake y Git instalados.

cmake -B build -DCOPY_AFTER_BUILD=OFF
if errorlevel 1 goto error

cmake --build build --config Release
if errorlevel 1 goto error

echo.
echo Listo. Copiando el plugin a la carpeta VST3 de tu usuario...
set "DEST=%LOCALAPPDATA%\Programs\Common\VST3"
if not exist "%DEST%" mkdir "%DEST%"
xcopy /E /I /Y "build\LowpassVisajoso_artefacts\Release\VST3\Lowpass Visajoso.vst3" "%DEST%\Lowpass Visajoso.vst3"

echo.
echo Plugin copiado a: %DEST%
echo Ahora abre FL Studio y haz "Find installed plugins" en el Plugin Manager.
pause
exit /b 0

:error
echo.
echo Algo fallo. Copia el mensaje de error de arriba y mandamelo.
pause
exit /b 1
