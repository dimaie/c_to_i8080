# Test script to demonstrate conditional runtime function inclusion

Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host "Testing Conditional Runtime Function Inclusion" -ForegroundColor Cyan
Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host ""

# Find the executable
$exe = $null
if (Test-Path ".\x64\Debug\c_to_i8080.exe") { 
    $exe = ".\x64\Debug\c_to_i8080.exe" 
} elseif (Test-Path ".\Debug\c_to_i8080.exe") { 
    $exe = ".\Debug\c_to_i8080.exe" 
} elseif (Test-Path ".\c_to_i8080.exe") { 
    $exe = ".\c_to_i8080.exe" 
}

if (-not $exe) {
    Write-Host "Error: Compiler executable not found!" -ForegroundColor Red
    exit 1
}

Write-Host "Using compiler: $exe" -ForegroundColor Green
Write-Host ""

# Test cases
$tests = @(
    @{Name="No Math Operations"; File="test_no_math.c"; Expected="Neither"},
    @{Name="Multiplication Only"; File="test_mul_only.c"; Expected="__mul only"},
    @{Name="Division Only"; File="test_div_only.c"; Expected="__div only"},
    @{Name="Both Operations"; File="test_both_math.c"; Expected="Both __mul and __div"}
)

foreach ($test in $tests) {
    Write-Host "------------------------------------------------------------------" -ForegroundColor Yellow
    Write-Host "Test: $($test.Name)" -ForegroundColor Yellow
    Write-Host "File: $($test.File)" -ForegroundColor Yellow
    Write-Host "Expected: $($test.Expected)" -ForegroundColor Yellow
    Write-Host "------------------------------------------------------------------" -ForegroundColor Yellow

    $output = "output_$($test.File.Replace('.c', '.asm'))"

    # Compile
    & $exe $test.File $output 2>&1 | Out-Null

    if (Test-Path $output) {
        # Check for runtime functions
        $content = Get-Content $output -Raw
        $hasMul = $content -match "__mul:"
        $hasDiv = $content -match "__div:"

        Write-Host "Runtime functions included:" -ForegroundColor Cyan
        if ($hasMul) {
            Write-Host "  [X] __mul (multiplication)" -ForegroundColor Green
        } else {
            Write-Host "  [ ] __mul (multiplication)" -ForegroundColor DarkGray
        }

        if ($hasDiv) {
            Write-Host "  [X] __div (division)" -ForegroundColor Green
        } else {
            Write-Host "  [ ] __div (division)" -ForegroundColor DarkGray
        }

        # Show file size
        $size = (Get-Item $output).Length
        Write-Host "Output size: $size bytes" -ForegroundColor Cyan

        Write-Host ""
    } else {
        Write-Host "Error: Compilation failed!" -ForegroundColor Red
        Write-Host ""
    }
}

Write-Host "==================================================================" -ForegroundColor Cyan
Write-Host "Test Complete!" -ForegroundColor Cyan
Write-Host "==================================================================" -ForegroundColor Cyan
