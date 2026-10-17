@echo off
setlocal

cd /d "%~dp0"

if not exist bin mkdir bin

:: Embed shaders into src\shaders.h
if exist "scripts\embed_shaders.ps1" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\embed_shaders.ps1"
)

:: Find MSVC x64 environment if cl is not in PATH
where cl >nul 2>nul
if %errorlevel% neq 0 (
    if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" (
        for /f "tokens=*" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -property installationPath') do (
            if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" (
                call "%%i\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
            )
        )
    )
)

where cl >nul 2>nul
if %errorlevel% equ 0 (
    echo [MSVC] Compiling OBJ Viewer...
    cl /nologo /O2 /W3 /EHsc /MD /D_WIN32_WINNT=0x0A00 /DWINVER=0x0A00 /Iinclude /Isrc src\main.cpp src\glad.cpp /Fo:bin\ /Fe:bin\OBJViewer.exe /link /SUBSYSTEM:CONSOLE /MACHINE:X64 /LIBPATH:lib glfw3.lib opengl32.lib gdi32.lib user32.lib shell32.lib
    if errorlevel 1 goto build_failed
    del bin\main.obj bin\glad.obj >nul 2>nul
    goto build_success
)

:: Fallback to MinGW 64-bit if MSVC is not available
if exist "C:\msys64\mingw64\bin\g++.exe" (
    set "PATH=C:\msys64\mingw64\bin;%PATH%"
) else if exist "C:\msys64\ucrt64\bin\g++.exe" (
    set "PATH=C:\msys64\ucrt64\bin;%PATH%"
)

where g++ >nul 2>nul
if %errorlevel% equ 0 (
    echo [MinGW] Compiling OBJ Viewer for x64 Windows 10/11...
    g++ -m64 -O2 -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -Iinclude -Isrc src/main.cpp src/glad.cpp -o bin/OBJViewer.exe -Llib -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32 -Wl,--subsystem,console -static-libgcc -static-libstdc++
    if errorlevel 1 goto build_failed
    goto build_success
)

echo [ERROR] No suitable x64 C++ compiler found (MSVC or MinGW64).
exit /b 1

:build_success
echo [SUCCESS] Built bin\OBJViewer.exe
exit /b 0

:build_failed
del bin\main.obj bin\glad.obj >nul 2>nul
echo [FAILED] Compilation failed.
exit /b 1
