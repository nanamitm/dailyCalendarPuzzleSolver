param(
    [string]$QtVersion        = "6.11.1",
    [string]$NdkVersion       = "27.2.12479018",
    [string]$QtRoot           = "C:\Qt",
    [string]$AndroidSdkRoot   = "$env:LOCALAPPDATA\Android\Sdk",
    [string]$JavaHome         = "C:\Program Files\Android\Android Studio\jbr",
    [string]$HostQtPath       = "",
    [string]$CMakePath        = "",
    [string]$NinjaPath        = ""
)

$ErrorActionPreference = "Stop"

$buildDir  = "build-android"
$qtPrefix  = Join-Path $QtRoot "$QtVersion\android_arm64_v8a"

# Detect host Qt (mingw_64 / msvc2022_64 / llvm-mingw_64)
$hostQtCandidates = @(
    (Join-Path $QtRoot "$QtVersion\mingw_64"),
    (Join-Path $QtRoot "$QtVersion\msvc2022_64"),
    (Join-Path $QtRoot "$QtVersion\llvm-mingw_64")
)
if (!$HostQtPath) {
    foreach ($candidate in $hostQtCandidates) {
        if (Test-Path "$candidate\lib\cmake\Qt6\Qt6Config.cmake") {
            $HostQtPath = $candidate
            break
        }
    }
}

$cmake = if ($CMakePath)  { $CMakePath }  elseif (Test-Path "$QtRoot\Tools\CMake_64\bin\cmake.exe") { "$QtRoot\Tools\CMake_64\bin\cmake.exe" } else { "cmake.exe" }
$ninja = if ($NinjaPath)  { $NinjaPath }  elseif (Test-Path "$QtRoot\Tools\Ninja\ninja.exe")        { "$QtRoot\Tools\Ninja\ninja.exe" }         else { "ninja.exe" }

$env:JAVA_HOME          = $JavaHome
$env:ANDROID_SDK_ROOT   = $AndroidSdkRoot
$env:ANDROID_NDK_ROOT   = "$env:ANDROID_SDK_ROOT\ndk\$NdkVersion"
$env:Path               = "$env:JAVA_HOME\bin;$env:ANDROID_SDK_ROOT\platform-tools;$env:Path"

if (!(Test-Path "$qtPrefix\lib\cmake\Qt6\qt.toolchain.cmake")) {
    throw "Qt Android kit not found: $qtPrefix"
}
if (!(Test-Path $env:ANDROID_NDK_ROOT)) {
    throw "Android NDK not found: $env:ANDROID_NDK_ROOT"
}

$configureArgs = @(
    "-S", "Android",
    "-B", $buildDir,
    "-G", "Ninja",
    "-DCMAKE_TOOLCHAIN_FILE=$qtPrefix\lib\cmake\Qt6\qt.toolchain.cmake",
    "-DANDROID_ABI=arm64-v8a",
    "-DANDROID_PLATFORM=latest",
    "-DANDROID_SDK_ROOT=$env:ANDROID_SDK_ROOT",
    "-DANDROID_NDK_ROOT=$env:ANDROID_NDK_ROOT",
    "-DCMAKE_MAKE_PROGRAM=$ninja",
    "-DCMAKE_BUILD_TYPE=Debug"
)
if ($HostQtPath) {
    $configureArgs += "-DQT_HOST_PATH=$HostQtPath"
}

$qtDebugApk    = ".\$buildDir\android-build\PuzzleSolverAndroid.apk"
$gradleDebugApk = ".\$buildDir\android-build\build\outputs\apk\debug\android-build-debug.apk"
$debugApk      = ".\$buildDir\PuzzleSolverAndroid-debug.apk"

& $cmake @configureArgs

# Remove stale build artefacts that confuse incremental APK packaging
$stalePaths = @(
    "$buildDir\android-build\libs",
    "$buildDir\android-build\build\intermediates\merged_jni_libs",
    "$buildDir\android-build\build\intermediates\merged_native_libs",
    "$buildDir\android-build\build\intermediates\stripped_native_libs",
    $qtDebugApk,
    $gradleDebugApk,
    $debugApk
)
foreach ($path in $stalePaths) {
    if (Test-Path $path) { Remove-Item -LiteralPath $path -Recurse -Force }
}

& $cmake --build $buildDir
$buildExitCode = $LASTEXITCODE

if (Test-Path $qtDebugApk) {
    Copy-Item -LiteralPath $qtDebugApk -Destination $debugApk -Force
    Write-Host "Debug APK: $debugApk"
    exit 0
}
if (Test-Path $gradleDebugApk) {
    Copy-Item -LiteralPath $gradleDebugApk -Destination $debugApk -Force
    Write-Host "Debug APK: $debugApk"
    exit 0
}

if ($buildExitCode -ne 0) {
    throw "Debug build failed with exit code $buildExitCode."
}
throw "Debug APK was not created."
