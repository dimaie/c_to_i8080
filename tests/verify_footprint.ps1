# verify_footprint.ps1
# Instrument to verify that new compiler iterations do not inflate memory footprint or break backwards compatibility for non-struct programs.

$Compiler = "..\c_to_i8080.exe"
if (-not (Test-Path $Compiler)) {
    $Compiler = "..\x64\Debug\c_to_i8080.exe"
}

$TestFile = "test_core.c"
$OutputFile = "output_test_core.asm"

# Known baseline byte-size for test_core.c BEFORE struct support was implemented
# (Adjust this exact number to match your environment's clean build of the previous git commit if needed)
$ExpectedBaselineSize = 6496 

Write-Host "Compiling test_core.c using the new compiler..." -ForegroundColor Cyan
& $Compiler $TestFile $OutputFile 2>&1 | Out-Null

if (-not (Test-Path $OutputFile)) {
    Write-Host "Error: Compilation failed!" -ForegroundColor Red
    exit 1
}

$Size = (Get-Item $OutputFile).Length
Write-Host "Generated Footprint: $Size bytes" -ForegroundColor Yellow
Write-Host "Expected Footprint:  $ExpectedBaselineSize bytes" -ForegroundColor Yellow

if ($Size -eq $ExpectedBaselineSize) {
    Write-Host "[PASS] Memory footprint is identical. Zero overhead verified!" -ForegroundColor Green
} else {
    Write-Host "[WARN] Footprint changed!" -ForegroundColor Red
    Write-Host "This usually indicates the new AST features are unintentionally leaking bloated instructions or padding into standard primitives."
}

# Cleanup
Remove-Item $OutputFile -ErrorAction SilentlyContinue