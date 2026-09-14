@echo off
setlocal

set BUILD_DIR=%~dp0
set QEMU=C:\msys64\mingw64\bin\qemu-system-i386.exe

echo ==========================================
echo  Hydra OS - Build and Run
echo ==========================================

:: Build the ISO using the MSYS2 toolchain (clang + lld + xorriso)
echo.
echo [1/2] Building redlion.iso ...
C:\msys64\usr\bin\bash.exe -lc "cd '%BUILD_DIR:C:\=/c/%' && make redlion.iso"
if errorlevel 1 (
    echo.
    echo BUILD FAILED.
    exit /b 1
)

echo.
echo [2/2] Launching QEMU ...
if not exist "%QEMU%" (
    echo QEMU not found at %QEMU%
    echo Install it with:  pacman -S mingw-w64-x86_64-qemu
    exit /b 1
)

"%QEMU%" -cdrom "%BUILD_DIR%redlion.iso" -m 128 -display sdl -no-reboot

echo.
echo Done.
endlocal
