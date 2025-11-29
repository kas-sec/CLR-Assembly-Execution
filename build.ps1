param(
    [switch]$Clean
)


$ErrorActionPreference = "Stop"


Write-Host "`n  CLR Assembly Execution - Build" -ForegroundColor Cyan
Write-Host "  ==============================`n" -ForegroundColor Cyan


if ($Clean -and (Test-Path "build")) {
    Write-Host "[*] cleaning..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build"
}


$clangCl = Get-Command clang-cl -ErrorAction SilentlyContinue
if (-not $clangCl) {
    Write-Host "[!] clang-cl not found" -ForegroundColor Red
    exit 1
}
Write-Host "[+] found clang-cl" -ForegroundColor Green


$sdkPath = "C:\Program Files (x86)\Windows Kits\10\bin"
$rcExe = Get-ChildItem -Path $sdkPath -Recurse -Filter "rc.exe" -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "\\x64\\rc\.exe$" } |
    Sort-Object { $_.FullName } -Descending |
    Select-Object -First 1


if (-not $rcExe) {
    Write-Host "[!] rc.exe not found" -ForegroundColor Red
    exit 1
}
Write-Host "[+] found rc.exe" -ForegroundColor Green


if ($rcExe.FullName -match "\\bin\\(10\.\d+\.\d+\.\d+)\\") {
    $sdkVersion = $matches[1]
    Write-Host "[+] SDK: $sdkVersion" -ForegroundColor Green
}


$sdkBase = "C:\Program Files (x86)\Windows Kits\10"


$vcBase = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"
if (-not (Test-Path $vcBase)) {
    $vcBase = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC"
}
if (-not (Test-Path $vcBase)) {
    $vcBase = "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Tools\MSVC"
}
if (-not (Test-Path $vcBase)) {
    $vcBase = "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC"
}


$vcVer = (Get-ChildItem $vcBase -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1).Name
Write-Host "[+] MSVC: $vcVer`n" -ForegroundColor Green


$inc = @(
    "/I`"$sdkBase\Include\$sdkVersion\um`"",
    "/I`"$sdkBase\Include\$sdkVersion\shared`"",
    "/I`"$sdkBase\Include\$sdkVersion\ucrt`"",
    "/I`"$vcBase\$vcVer\include`""
)


$lib = @(
    "/LIBPATH:`"$sdkBase\Lib\$sdkVersion\um\x64`"",
    "/LIBPATH:`"$sdkBase\Lib\$sdkVersion\ucrt\x64`"",
    "/LIBPATH:`"$vcBase\$vcVer\lib\x64`""
)


$crtLibs = @(
    "kernel32.lib",
    "user32.lib",
    "libcmt.lib",
    "libcpmt.lib",
    "libvcruntime.lib",
    "libucrt.lib"
)


@("build", "outputs") | ForEach-Object {
    if (-not (Test-Path $_)) { New-Item -ItemType Directory -Path $_ | Out-Null }
}


Write-Host "[1/4] compiling loader (release)" -ForegroundColor Yellow
$loaderArgs = @(
    "/nologo", "/EHsc", "/std:c++17", "/Od", "/MT", "-w",
    "/D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH",
    "/DWIN32", "/D_WINDOWS"
) + $inc + @(
    "loader\main.cpp",
    "loader\clr.cpp",
    "common\crypto.cpp",
    "/link",
    "/OUT:build\loader_stub.exe",
    "/SUBSYSTEM:WINDOWS"
) + $lib + $crtLibs + @(
    "ole32.lib",
    "oleaut32.lib",
    "bcrypt.lib",
    "advapi32.lib"
)


$p = Start-Process -FilePath "clang-cl" -ArgumentList $loaderArgs -NoNewWindow -Wait -PassThru
if ($p.ExitCode -ne 0) {
    Write-Host "[!] loader (release) failed" -ForegroundColor Red
    exit 1
}
Write-Host "    done" -ForegroundColor Green


Write-Host "[2/4] compiling loader (debug)" -ForegroundColor Yellow
$loaderDebugArgs = @(
    "/nologo", "/EHsc", "/std:c++17", "/Od", "/MT", "-w",
    "/D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH",
    "/DWIN32", "/D_WINDOWS", "/DDEBUG_BUILD"
) + $inc + @(
    "loader\main.cpp",
    "loader\clr.cpp",
    "common\crypto.cpp",
    "/link",
    "/OUT:build\loader_stub_debug.exe",
    "/SUBSYSTEM:CONSOLE"
) + $lib + $crtLibs + @(
    "ole32.lib",
    "oleaut32.lib",
    "bcrypt.lib",
    "advapi32.lib"
)


$p = Start-Process -FilePath "clang-cl" -ArgumentList $loaderDebugArgs -NoNewWindow -Wait -PassThru
if ($p.ExitCode -ne 0) {
    Write-Host "[!] loader (debug) failed" -ForegroundColor Red
    exit 1
}
Write-Host "    done" -ForegroundColor Green


Write-Host "[3/4] compiling resources" -ForegroundColor Yellow
$p = Start-Process -FilePath $rcExe.FullName -ArgumentList @(
    "/nologo", "/fo", "build\loader_stub.res", "builder\res\loader_stub.rc"
) -NoNewWindow -Wait -PassThru


if ($p.ExitCode -ne 0) {
    Write-Host "[!] resources failed" -ForegroundColor Red
    exit 1
}
Write-Host "    done" -ForegroundColor Green


Write-Host "[4/4] compiling builder" -ForegroundColor Yellow
$builderArgs = @(
    "/nologo", "/EHsc", "/std:c++17", "/Od", "/MT", "-w",
    "/D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH",
    "/DWIN32", "/D_WINDOWS"
) + $inc + @(
    "builder\main.cpp",
    "builder\cli\args.cpp",
    "builder\core\payload.cpp",
    "common\crypto.cpp",
    "build\loader_stub.res",
    "/link",
    "/OUT:build\builder.exe",
    "/SUBSYSTEM:CONSOLE"
) + $lib + $crtLibs + @(
    "bcrypt.lib"
)


$p = Start-Process -FilePath "clang-cl" -ArgumentList $builderArgs -NoNewWindow -Wait -PassThru
if ($p.ExitCode -ne 0) {
    Write-Host "[!] builder failed" -ForegroundColor Red
    exit 1
}
Write-Host "    done" -ForegroundColor Green


Write-Host "`n  build finished!" -ForegroundColor Green
Write-Host "  ===============" -ForegroundColor Green
Write-Host ""
Write-Host "  Usage:"
Write-Host "    .\build\builder.exe -i <assembly> -o <name>"
Write-Host "    .\build\builder.exe -i <assembly> -o <name> --debug"
Write-Host ""
