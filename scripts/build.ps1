param([ValidateSet('Release','Debug')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
$source=Split-Path -Parent $PSScriptRoot
$command=Get-Command cmake -ErrorAction SilentlyContinue
$cmake=if($command){$command.Source}else{$null}
if(-not $cmake){
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if(Test-Path -LiteralPath $vswhere){$cmake=& $vswhere -latest -products '*' -find 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' | Select-Object -First 1}
}
if(-not $cmake){throw 'Install CMake 3.25+ or the Visual Studio CMake tools.'}
$ctest=Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
Push-Location $source
try {
    python scripts/check_vendor.py
    if($LASTEXITCODE -ne 0){throw 'Dependency hash check failed'}
    & $cmake --preset windows -DBUILD_TESTING=ON
    if($LASTEXITCODE -ne 0){throw 'CMake configure failed (Visual Studio 2022 C++ tools and Windows SDK required)'}
    & $cmake --build --preset ('windows-'+$Configuration.ToLowerInvariant()) --parallel
    if($LASTEXITCODE -ne 0){throw 'RiffCalc build failed'}
    & $ctest --preset ('windows-'+$Configuration.ToLowerInvariant())
    if($LASTEXITCODE -ne 0){throw 'RiffCalc tests failed'}
} finally {Pop-Location}
