param([string]$AfterClose = '')
$ErrorActionPreference = 'Continue'
$scratch = Split-Path -Parent $MyInvocation.MyCommand.Path
$proj = 'C:\Users\User\Documents\Unreal Projects\AmnesiaBeta\AmnesiaBeta.uproject'

# save everything through the MCP bridge if the editor is up
try { Push-Location $scratch; python -c "import ue; print('play session running, nothing to save here') if ue.pie_on() else (ue.call(ue.AS,'save_assets',asset_paths=[]), print(ue.job([{'op':'save'}], timeout=300)), print('saved'))"; Pop-Location } catch { "save skipped" }

# close the main project editor (and any other editor, since Live Coding blocks the build)
Get-Process UnrealEditor -ErrorAction SilentlyContinue | ForEach-Object { $_.CloseMainWindow() | Out-Null; if (-not $_.WaitForExit(120000)) { "editor $($_.Id) did not close, asking again" } }
# a background editor sometimes ignores the first close request
Get-Process UnrealEditor -ErrorAction SilentlyContinue | ForEach-Object { $_.CloseMainWindow() | Out-Null; if (-not $_.WaitForExit(90000)) { "editor $($_.Id) still open" } }
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { "an editor is still running, not building"; exit 1 }
Get-Process LiveCodingConsole -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue

# a step that needs the editor closed (a python script, run from here), e.g. remove_folder_files.py LW
if ($AfterClose) { Push-Location $scratch; Invoke-Expression $AfterClose; Pop-Location }

& 'D:\UE_5.8\Engine\Build\BatchFiles\Build.bat' AmnesiaBetaEditor Win64 Development "-Project=$proj" -WaitMutex 2>&1 | Select-String 'error|warning C|Result|Unable'
if ($LASTEXITCODE -ne 0) { "build failed (exit $LASTEXITCODE), the editor is not started"; exit 1 }

Start-Process 'D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList "`"$proj`""
for ($i = 0; $i -lt 90; $i++) {
    Start-Sleep 5
    if (Get-NetTCPConnection -State Listen -LocalPort 8000 -ErrorAction SilentlyContinue) { "editor up after $($i * 5 + 5)s"; break }
}
Start-Sleep 20
