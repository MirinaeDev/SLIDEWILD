param([string]$Executable = 'Slidewild.exe', [switch]$NativeOnly, [int]$Jobs = 2)
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$bin = Join-Path $root '.tools/w64devkit/bin'
$cmake = Join-Path $bin 'cmake.exe'
$ctest = Join-Path $bin 'ctest.exe'
$sdl = Join-Path $root '.tools/sdl/SDL3-3.4.18/x86_64-w64-mingw32'
$dxc = Join-Path $root '.tools/dxc/bin/x64/dxc.exe'
foreach ($tool in @($cmake,$ctest,$dxc,(Join-Path $bin 'g++.exe'))) {
    if (!(Test-Path -LiteralPath $tool)) { throw "Missing tool: $tool. Run ./setup.ps1." }
}
if ($Jobs -lt 1) { throw 'Jobs must be positive' }
if ([IO.Path]::GetFileName($Executable) -ne $Executable) { throw 'Executable must be a filename' }
$build = Join-Path $root 'build/windows'
$testing = if ($NativeOnly) { 'OFF' } else { 'ON' }
& $cmake -S $root -B $build -G 'MinGW Makefiles' `
    "-DCMAKE_MAKE_PROGRAM=$bin/make.exe" "-DCMAKE_C_COMPILER=$bin/gcc.exe" `
    "-DCMAKE_CXX_COMPILER=$bin/g++.exe" "-DSDL3_DIR=$sdl/lib/cmake/SDL3" `
    "-DDXC_EXECUTABLE=$dxc" -DDH_FETCH_SDL=OFF -DCMAKE_BUILD_TYPE=Release "-DBUILD_TESTING=$testing"
if ($LASTEXITCODE) { throw 'CMake configuration failed' }
& $cmake --build $build --parallel $Jobs
if ($LASTEXITCODE) { throw 'CMake build failed' }
if (!$NativeOnly) {
    & $ctest --test-dir $build --output-on-failure
    if ($LASTEXITCODE) { throw 'CTest failed' }
}
# Keep the existing launchers and release scripts working with the canonical CMake build.
Copy-Item -LiteralPath "$build/Slidewild.exe" -Destination "$root/build/$Executable" -Force
Copy-Item -LiteralPath "$sdl/bin/SDL3.dll" -Destination "$root/build/SDL3.dll" -Force
& $cmake "-DROOT=$root" "-DDEST=$root/build" "-DSHADER_DIR=$build/shaders" -DSHADER_EXT=dxil -P "$root/cmake/StageResources.cmake"
if ($LASTEXITCODE) { throw 'Resource staging failed' }
Write-Host "Build OK : build/$Executable"
