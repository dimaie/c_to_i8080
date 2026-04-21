@echo off
echo Testing C to i8080 Compiler with Inline Assembly
echo.

set EXE=
if exist ".\x64\Debug\c_to_i8080.exe" set EXE=.\x64\Debug\c_to_i8080.exe
if exist ".\Debug\c_to_i8080.exe" set EXE=.\Debug\c_to_i8080.exe  
if exist ".\c_to_i8080.exe" set EXE=.\c_to_i8080.exe

if "%EXE%"=="" (
    echo Error: Compiler not found
    exit /b 1
)

echo Using: %EXE%
echo.

echo Test 1: Simple inline assembly
%EXE% test_simple_asm.c output_simple.asm
if %ERRORLEVEL%==0 (
    echo [OK] Compilation succeeded
    type output_simple.asm
) else (
    echo [FAIL] Compilation failed
)

pause
