param(
    [ValidateSet("d3d12", "dxgi", "all")]
    [string]$target = "all",
    [ValidateSet("standard", "enthusiast")]
    [string]$profile = "standard"
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$SrcRoot = Join-Path $Root ("src\" + $profile)
$OutDir = Join-Path $Root ("proxy_build\" + $profile)

if (!(Test-Path $SrcRoot)) {
    Write-Error "Perfil de codigo fonte nao encontrado: $SrcRoot"
}

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

function Build-Target([string]$name) {
    if ($name -eq "dxgi") {
        $src = Join-Path $SrcRoot "DXGIProxy.cpp"
        $asm = Join-Path $SrcRoot "DXGIProxyStubs.asm"
        $def = Join-Path $SrcRoot "dxgi_proxy.def"
        $out = Join-Path $OutDir "dxgi.dll"
        $obj = Join-Path $OutDir "DXGIProxy.obj"
        $asmObj = Join-Path $OutDir "DXGIProxyStubs.obj"
    } else {
        $src = Join-Path $SrcRoot "D3D12Proxy.cpp"
        $asm = Join-Path $SrcRoot "D3D12ProxyStubs.asm"
        $def = Join-Path $SrcRoot "d3d12_proxy.def"
        $out = Join-Path $OutDir "d3d12.dll"
        $obj = Join-Path $OutDir "D3D12Proxy.obj"
        $asmObj = Join-Path $OutDir "D3D12ProxyStubs.obj"
    }

    foreach ($path in @($src, $asm, $def)) {
        if (!(Test-Path $path)) {
            Write-Error "Arquivo obrigatorio nao encontrado para '$name': $path"
        }
    }

    if ($vcvars) {
        cmd /c "`"$vcvars`" >nul && ml64.exe /nologo /c /Fo `"$asmObj`" `"$asm`" && cl.exe /nologo /EHsc /std:c++17 /LD `"$src`" `"$asmObj`" /Fe:`"$out`" /Fo:`"$obj`" /link /DEF:`"$def`""
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        return
    }

    if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
        & ml64.exe /nologo /c /Fo $asmObj $asm
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & cl.exe /nologo /EHsc /std:c++17 /LD $src $asmObj /Fe:$out /Fo:$obj /link /DEF:$def
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        return
    }

    Write-Error "cl.exe nao encontrado."
}

if ($target -eq "all") {
    Build-Target "d3d12"
    Build-Target "dxgi"
    exit 0
}

Build-Target $target
exit 0

