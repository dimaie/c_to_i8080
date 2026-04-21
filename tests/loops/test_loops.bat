@echo off
echo ========================================
echo Testing Loop Support
echo ========================================
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

echo Test 1: For Loop
echo ----------------------------------------
%EXE% test_for_loop.c output_for.asm
if %ERRORLEVEL%==0 (
    echo [OK] For loop compiled
    findstr /C:".L" output_for.asm | find /C ".L" >nul
    if %ERRORLEVEL%==0 echo [OK] Loop labels generated
) else (
    echo [FAIL] Compilation failed
)
echo.

echo Test 2: For Loop with Declaration
echo ----------------------------------------
%EXE% test_for_with_decl.c output_for_decl.asm
if %ERRORLEVEL%==0 (
    echo [OK] For loop with declaration compiled
) else (
    echo [FAIL] Compilation failed
)
echo.

echo Test 3: Do-While Loop
echo ----------------------------------------
%EXE% test_do_while.c output_do_while.asm
if %ERRORLEVEL%==0 (
    echo [OK] Do-while loop compiled
    findstr /C:".L" output_do_while.asm | find /C ".L" >nul
    if %ERRORLEVEL%==0 echo [OK] Loop labels generated
) else (
    echo [FAIL] Compilation failed
)
echo.

echo Test 4: Nested Loops
echo ----------------------------------------
%EXE% test_nested_loops.c output_nested.asm
if %ERRORLEVEL%==0 (
    echo [OK] Nested loops compiled
) else (
    echo [FAIL] Compilation failed
)
echo.

echo Test 5: All Loop Types
echo ----------------------------------------
%EXE% test_all_loops.c output_all_loops.asm
if %ERRORLEVEL%==0 (
    echo [OK] All loop types compiled
    echo.
    echo Generated assembly snippet:
    findstr /C:"while\|for\|do" test_all_loops.c
) else (
    echo [FAIL] Compilation failed
)
echo.

echo ========================================
echo All Loop Tests Complete!
echo ========================================
pause
