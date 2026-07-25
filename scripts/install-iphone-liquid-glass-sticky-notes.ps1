<#
.SYNOPSIS
  Beginner-friendly helper for installing iPhone Sticky Notes into Windhawk.

.DESCRIPTION
  Windhawk currently has command-line flags for starting/restarting/exiting the app,
  but not a stable official CLI command for importing and enabling an arbitrary local
  .wh.cpp mod. This script automates the safe parts:
    1. Checks that it is running on Windows.
    2. Installs Windhawk with winget if Windhawk is missing.
    3. Copies the mod source code to the clipboard.
    4. Opens Windhawk so the user only needs to create a new mod, paste, compile,
       and enable it.
#>

param(
    [switch]$SkipWindhawkInstall
)

$ErrorActionPreference = 'Stop'

function Write-Step {
    param([string]$Text)
    Write-Host "`n==> $Text" -ForegroundColor Cyan
}

function Find-WindhawkExe {
    $candidates = @(
        "$env:ProgramFiles\Windhawk\windhawk.exe",
        "${env:ProgramFiles(x86)}\Windhawk\windhawk.exe",
        "$env:LocalAppData\Programs\Windhawk\windhawk.exe"
    )

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return $candidate
        }
    }

    $command = Get-Command windhawk.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    return $null
}

if (-not $IsWindows) {
    throw 'Этот скрипт нужно запускать на Windows, потому что Windhawk работает на Windows.'
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$modPath = Join-Path $repoRoot 'windhawk\iphone-liquid-glass-sticky-notes.wh.cpp'

if (-not (Test-Path $modPath)) {
    throw "Не найден файл мода: $modPath"
}

Write-Step 'Проверяю Windhawk'
$windhawkExe = Find-WindhawkExe

if (-not $windhawkExe -and -not $SkipWindhawkInstall) {
    Write-Step 'Windhawk не найден. Пробую установить через winget'
    $winget = Get-Command winget.exe -ErrorAction SilentlyContinue
    if (-not $winget) {
        throw 'winget не найден. Установите Windhawk вручную с https://windhawk.net/ и запустите скрипт снова.'
    }

    & winget install --id RamenSoftware.Windhawk --source winget --accept-package-agreements --accept-source-agreements
    $windhawkExe = Find-WindhawkExe
}

if (-not $windhawkExe) {
    throw 'Windhawk не найден. Установите Windhawk и запустите скрипт снова.'
}

Write-Step 'Копирую код мода в буфер обмена'
Get-Content -Raw -Encoding UTF8 $modPath | Set-Clipboard

Write-Step 'Открываю Windhawk'
Start-Process -FilePath $windhawkExe

Write-Host @'

ГОТОВО. Дальше в Windhawk осталось сделать 4 клика:

1. Нажмите Mods.
2. Нажмите Create a new mod / Создать новый мод.
3. Удалите пример кода и нажмите Ctrl+V — код уже в буфере обмена.
4. Нажмите Compile Mod, затем Exit Editing Mode и включите мод.

После включения мода на экране появится синяя кнопка +. Нажмите +, чтобы создать стикер.
'@ -ForegroundColor Green
