$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$OutDir = Join-Path $Root "caps_build"
$Src = Join-Path $Root "D3D12Caps.cpp"
$Out = Join-Path $OutDir "D3D12Caps.exe"
$Obj = Join-Path $OutDir "D3D12Caps.obj"

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"

cmd /c "`"$vcvars`" >nul && cl.exe /nologo /EHsc /std:c++17 `"$Src`" /Fe:`"$Out`" /Fo:`"$Obj`" /link d3d12.lib dxgi.lib"
exit $LASTEXITCODE
