param(
    [string]$KeystorePath     = ".\keystore\puzzlesolver-release.jks",
    [string]$Alias            = "puzzlesolver",
    [string]$CommonName       = "Puzzle Solver",
    [string]$OrganizationalUnit = "Personal",
    [string]$Organization     = "nanamitm",
    [string]$Country          = "JP",
    [int]   $ValidityDays     = 10950   # 30 years
)

$ErrorActionPreference = "Stop"

$keytool = "C:\Program Files\Android\Android Studio\jbr\bin\keytool.exe"
if (!(Test-Path $keytool)) { throw "keytool not found: $keytool" }

$keystoreFullPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($KeystorePath)
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $keystoreFullPath) | Out-Null

if (Test-Path $keystoreFullPath) { throw "Keystore already exists: $keystoreFullPath" }

$storePassword = Read-Host "Keystore password" -AsSecureString
$keyPassword   = Read-Host "Key password"      -AsSecureString
$storePasswordText = [System.Net.NetworkCredential]::new("", $storePassword).Password
$keyPasswordText   = [System.Net.NetworkCredential]::new("", $keyPassword).Password

& $keytool -genkeypair -v `
    -keystore $keystoreFullPath -storetype JKS `
    -alias $Alias -keyalg RSA -keysize 2048 -validity $ValidityDays `
    -dname "CN=$CommonName, OU=$OrganizationalUnit, O=$Organization, C=$Country" `
    -storepass $storePasswordText -keypass $keyPasswordText

$content = "storeFile=$KeystorePath`nstorePassword=$storePasswordText`nkeyAlias=$Alias`nkeyPassword=$keyPasswordText`n"
[System.IO.File]::WriteAllText((Join-Path (Resolve-Path ".").Path "android-signing.properties"), $content, [System.Text.UTF8Encoding]::new($false))

Write-Host "Created keystore : $keystoreFullPath"
Write-Host "Created          : android-signing.properties"
Write-Host "Keep both private. Do NOT commit them."
