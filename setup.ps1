# Portable, pinned Windows toolchain. Downloads only from upstream release repositories.
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$download = Join-Path $root '.tools\downloads'
New-Item -ItemType Directory -Force $download | Out-Null
$packages = @(
    @{Name='w64devkit.exe'; Url='https://github.com/skeeto/w64devkit/releases/download/v2.10.0/w64devkit-x64-2.10.0.7z.exe'; Hash='18D0A4C71A166F8401AB6305781BEC5882B40B5E06BA9807C61CB5F3B3C6325E'},
    @{Name='SDL3.zip'; Url='https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-devel-3.4.18-mingw.zip'; Hash='CD98158C8D025A816F430A600B4E9526524E6219EE884EC72BD498B80FD3BF58'},
    @{Name='dxc.zip'; Url='https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2609/dxc_2026_09_29.zip'; Hash='AD31B1FC8443175D204F77A611FDB3EF2EC42759BDC2F1167368DE24A4A7E7F1'}
)
foreach ($package in $packages) {
    $file = Join-Path $download $package.Name
    if (!(Test-Path -LiteralPath $file)) { Invoke-WebRequest $package.Url -OutFile $file }
    if ((Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash -ne $package.Hash) { throw "Archive checksum mismatch: $file" }
}
Expand-Archive "$download\SDL3.zip" "$root\.tools\sdl" -Force
Expand-Archive "$download\dxc.zip" "$root\.tools\dxc" -Force
& "$download\w64devkit.exe" -y "-o$root\.tools"
if ($LASTEXITCODE) { throw 'Compiler extraction failed' }
Write-Host 'Toolchain ready. Run ./build.ps1'
