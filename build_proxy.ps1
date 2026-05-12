$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$OutDir = Join-Path $Root "proxy_build"
$Src = Join-Path $Root "src\D3D12Proxy.cpp"
$Asm = Join-Path $Root "src\D3D12ProxyStubs.asm"
$Def = Join-Path $Root "src\d3d12_proxy.def"
$Out = Join-Path $OutDir "d3d12.dll"
$Obj = Join-Path $OutDir "D3D12Proxy.obj"
$AsmObj = Join-Path $OutDir "D3D12ProxyStubs.obj"

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$vswhereCandidates = @(
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
)

$vcvars = $null
$vswhere = $vswhereCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vsPath) {
        $candidate = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $candidate) {
            $vcvars = $candidate
        }
    }
}

if ($vcvars) {
    cmd /c "`"$vcvars`" >nul && ml64.exe /nologo /c /Fo `"$AsmObj`" `"$Asm`" && cl.exe /nologo /EHsc /std:c++17 /LD `"$Src`" `"$AsmObj`" /Fe:`"$Out`" /Fo:`"$Obj`" /link /DEF:`"$Def`""
    exit $LASTEXITCODE
}

if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    & ml64.exe /nologo /c /Fo $AsmObj $Asm
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    & cl.exe /nologo /EHsc /std:c++17 /LD $Src $AsmObj /Fe:$Out /Fo:$Obj /link /DEF:$Def
    exit $LASTEXITCODE
}

Write-Error "cl.exe nao encontrado."
