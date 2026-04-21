@echo off
echo =========================================
echo Testing Inline Assembly Support
echo =========================================
echo.

REM Find the compiler executable
set EXE=
if exist ".\x64\Debug\c_to_i8080.exe" set EXE=.\x64\Debug\c_to_i8080.exe
if exist ".\Debug\c_to_i8080.exe" set EXE=.\Debug\c_to_i8080.exe
if exist ".\c_to_i8080.exe" set EXE=.\c_to_i8080.exe

if "%EXE%"=="" (
    echo Error: Compiler executable not found!
    exit /b 1
)

echo Using compiler: %EXE%
echo.

echo -----------------------------------------
echo Test 1: Basic Inline Assembly
echo -----------------------------------------
%EXE% test_asm_basic.c output_asm_basic.asm
if exist output_asm_basic.asm (
    echo [OK] Compiled successfully
    findstr /C:"Inline assembly" output_asm_basic.asm >nul
    if %ERRORLEVEL%==0 (
        echo [OK] Contains inline assembly marker
    )
    findstr /C:"MVI A, 0xFF" output_asm_basic.asm >nul
    if %ERRORLEVEL%==0 (
        echo [OK] Assembly instructions present
    )
) else (
    echo [FAIL] Compilation failed
)
echo.

echo -----------------------------------------
echo Test 2: Advanced Inline Assembly
echo -----------------------------------------
%EXE% test_asm_advanced.c output_asm_advanced.asm
if exist output_asm_advanced.asm (
    echo [OK] Compiled successfully
    findstr /C:"IN 20H" output_asm_advanced.asm >nul
    if %ERRORLEVEL%==0 (
        echo [OK] I/O instructions present
    )
    findstr /C:"LOOP:" output_asm_advanced.asm >nul
    if %ERRORLEVEL%==0 (
        echo [OK] Labels present
    )
) else (
    echo [FAIL] Compilation failed
)
echo.

echo -----------------------------------------
echo Test 3: Mixed C and Assembly
echo -----------------------------------------
%EXE% test_asm_mixed.c output_asm_mixed.asm
if exist output_asm_mixed.asm (
    echo [OK] Compiled successfully
    findstr /C:"add:" output_asm_mixed.asm >nul
    if %ERRORLEVEL%==0 (
        echo [OK] C function present
    )
    findstr /C:"Inline assembly" output_asm_mixed.asm >nul
    if %ERRORLEVEL%==0 (
        echo [OK] Inline assembly present
    )
    findstr /C:"__VAR_sum" output_asm_mixed.asm >nul
    if %ERRORLEVEL%==0 (
        echo [OK] Variable access present
    )
) else (
    echo [FAIL] Compilation failed
)
echo.

echo =========================================
echo All Tests Complete!
echo =========================================
