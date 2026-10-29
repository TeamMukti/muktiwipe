# muktiwipe functional test suite
# usage:  powershell -NoProfile -ExecutionPolicy Bypass -File tests\muktiwipe_test.ps1
$ErrorActionPreference = 'Continue'
$exe  = Join-Path (Split-Path $PSScriptRoot -Parent) "muktiwipe.exe"

if (-not (Test-Path $exe)) { Write-Output "muktiwipe.exe not found - run build.bat first"; exit 1 }

# the test tree must live OUTSIDE C:\Users and C:\Windows, which muktiwipe
# blocks by design (restricted paths)
$base = "C:\muktiwipe_test"
$X = @()
try { New-Item -ItemType Directory -Path $base -Force | Out-Null } catch { }
if (-not (Test-Path $base)) {
    $base = Join-Path $env:TEMP "muktiwipe_test"
    $X = @("-x")
}

# kill any locker process left over by an earlier (crashed) suite run
Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" |
    Where-Object { $_.CommandLine -match 'muktiwipe_lock' } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }

$failures = 0

function Check($name, $ok, $detail) {
    if ($ok) { Write-Output ("PASS  " + $name) }
    else     { Write-Output ("FAIL  " + $name + "  -> " + $detail); $script:failures++ }
}

function New-TestTree {
    if (Test-Path $base) { Remove-Item -LiteralPath $base -Recurse -Force }
    New-Item -ItemType Directory -Path "$base\dir1\sub1","$base\dir1\sub2","$base\dir2","$base\empty" -Force | Out-Null
    for ($i=0; $i -lt 500; $i++) { Set-Content -Path "$base\dir1\sub1\f$i.txt" -Value ("x" * 100) }
    for ($i=0; $i -lt 500; $i++) { Set-Content -Path "$base\dir1\sub2\f$i.txt" -Value ("y" * 200) }
    for ($i=0; $i -lt 500; $i++) { Set-Content -Path "$base\dir2\f$i.bin" -Value ("z" * 300) }
    Set-Content "$base\dir2\ro.txt" "ro"; (Get-Item "$base\dir2\ro.txt").Attributes = 'ReadOnly'
    Set-Content "$base\dir2\hid.txt" "hid"; (Get-Item "$base\dir2\hid.txt").Attributes = 'Hidden'
    Set-Content "$base\dir2\sys.txt" "sys"; (Get-Item "$base\dir2\sys.txt").Attributes = 'System'
    $uni = "f_" + [char]0x00E9 + [char]0x00FC + [char]0x044F + [char]0x2014 + ".txt"
    Set-Content "$base\dir1\$uni" "unicode"
    New-Item -ItemType Junction -Path "$base\dir2\junction" -Target "$base\dir1\sub1" | Out-Null
}

$lockcmd = '$f=[IO.File]::Open("C:\muktiwipe_test\dir2\f0.bin","Open","Read","None"); Start-Sleep 60; $f.Close(); #muktiwipe_lock'
$lockenc = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($lockcmd))

function Start-Locker {
    return Start-Process powershell -ArgumentList "-NoProfile","-EncodedCommand",$lockenc -WindowStyle Hidden -PassThru
}

function Stop-Locker($sleeper) {
    if ($sleeper -and -not $sleeper.HasExited) {
        Stop-Process -Id $sleeper.Id -Force -ErrorAction SilentlyContinue
        $sleeper.WaitForExit(5000) | Out-Null
    }
    Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" |
        Where-Object { $_.CommandLine -match 'muktiwipe_lock' } |
        ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
}

Write-Output "Test tree: $base"

Write-Output "=== T1: no arguments -> exit 50 ==="
& $exe 2>&1 | Out-Null
Check "T1 exit code 50" ($LASTEXITCODE -eq 50) ("got " + $LASTEXITCODE)

Write-Output "=== T2: root of drive -> exit 62 ==="
& $exe C:\ 2>&1 | Out-Null
Check "T2 exit code 62" ($LASTEXITCODE -eq 62) ("got " + $LASTEXITCODE)

Write-Output "=== T3: nonexistent path -> exit 60 ==="
& $exe "$env:TEMP\nope_muktiwipe_missing" 2>&1 | Out-Null
Check "T3 exit code 60" ($LASTEXITCODE -eq 60) ("got " + $LASTEXITCODE)

Write-Output "=== T4: restricted path (absolute) -> exit 63 ==="
& $exe C:\Windows\Temp 2>&1 | Out-Null
Check "T4 exit code 63" ($LASTEXITCODE -eq 63) ("got " + $LASTEXITCODE)

Write-Output "=== T5: restricted path (relative trick) -> exit 63 ==="
Push-Location C:\Windows
& $exe Temp 2>&1 | Out-Null
Pop-Location
Check "T5 exit code 63" ($LASTEXITCODE -eq 63) ("got " + $LASTEXITCODE)

Write-Output "=== T6: restricted path bypassed by -x stays safe (confirm declined with -j) ==="
"n" | & $exe -x -j C:\Windows\Temp 2>&1 | Out-Null
Check "T6 exit code 52" ($LASTEXITCODE -eq 52) ("got " + $LASTEXITCODE)
Check "T6 C:\Windows\Temp still there" (Test-Path C:\Windows\Temp) "missing"

Write-Output "=== T7: preview -> counts, nothing deleted, exit 0 ==="
New-TestTree
$out = & $exe @X -p -b $base 2>&1
Check "T7 exit code 0" ($LASTEXITCODE -eq 0) ("got " + $LASTEXITCODE)
Check "T7 tree still present" (Test-Path $base) "tree missing after preview"
$found = ($out | Select-String "1504").Count
Check "T7 reports 1504 files found" ($found -ge 1) ("output: " + ($out -join " | "))

Write-Output "=== T8: default delete (junction in tree, no -x) -> exit 0, tree gone ==="
New-TestTree
$t0 = Get-Date
$out = & $exe -y -b $base 2>&1
$dt = ((Get-Date) - $t0).TotalSeconds
Check "T8 exit code 0" ($LASTEXITCODE -eq 0) ("got " + $LASTEXITCODE)
Check "T8 tree deleted" (-not (Test-Path $base)) "tree still there"
Write-Output ("      (delete of ~1506 files took " + [math]::Round($dt,2) + " s)")

Write-Output "=== T9: staged + NT-API + keep-folder -> folder remains empty ==="
New-TestTree
$out = & $exe @X -y -s -n -1 -b -k $base 2>&1
Check "T9 exit code 0" ($LASTEXITCODE -eq 0) ("got " + $LASTEXITCODE)
Check "T9 folder kept" (Test-Path $base) "folder was removed"
$left = @(Get-ChildItem -LiteralPath $base -Force -Recurse).Count
Check "T9 folder empty" ($left -eq 0) ("left " + $left + " items")
Remove-Item -LiteralPath $base -Recurse -Force

Write-Output "=== T10: omni-delete on a file -> exit 0, file gone ==="
New-TestTree
$uni = "f_" + [char]0x00E9 + [char]0x00FC + [char]0x044F + [char]0x2014 + ".txt"
& $exe -o -y "$base\dir1\$uni" 2>&1 | Out-Null
Check "T10 exit code 0" ($LASTEXITCODE -eq 0) ("got " + $LASTEXITCODE)
Check "T10 file deleted" (-not (Test-Path "$base\dir1\$uni")) "file still there"
Remove-Item -LiteralPath $base -Recurse -Force

Write-Output "=== T11: locked file -> errors listed, exit 10 ==="
New-TestTree
$fs = [System.IO.File]::Open("$base\dir2\f0.bin",'Open','Read','None')
$out = & $exe @X -y -e $base 2>&1
$rc = $LASTEXITCODE
$fs.Close()
Check "T11 exit code 10" ($rc -eq 10) ("got " + $rc)
$errs = ($out | Select-String "Errors|Erreurs").Count
Check "T11 errors listed" ($errs -ge 1) "no errors block"
Remove-Item -LiteralPath $base -Recurse -Force

Write-Output "=== T12: long path (>260 chars) -> deleted ==="
New-TestTree
$deep = $base
for ($i=0; $i -lt 35; $i++) { $deep = $deep + "\longpath_dir" }
[System.IO.Directory]::CreateDirectory("\\?\" + $deep) | Out-Null
[System.IO.File]::WriteAllText("\\?\" + $deep + "\deep.txt", "deep")
Check "T12 long path created" ([System.IO.Directory]::Exists("\\?\" + $deep)) "creation failed"
$out = & $exe @X -y $base 2>&1
Check "T12 exit code 0" ($LASTEXITCODE -eq 0) ("got " + $LASTEXITCODE)
Check "T12 long tree deleted" (-not (Test-Path $base)) "tree still there"

Write-Output "=== T13: confirm prompt (-j) accepts 'y' ==="
New-TestTree
"y" | & $exe @X -j $base 2>&1 | Out-Null
Check "T13 exit code 0" ($LASTEXITCODE -eq 0) ("got " + $LASTEXITCODE)
Check "T13 tree deleted" (-not (Test-Path $base)) "tree still there"

Write-Output "=== T14: hostile ACL (deny Everyone) - fails without -f, deleted with -f ==="
New-TestTree
icacls $base /inheritance:r /deny "*S-1-1-0:(OI)(CI)(F)" | Out-Null
& $exe @X -y $base 2>&1 | Out-Null
$rc_plain = $LASTEXITCODE
Check "T14 fails without -f (RC >= 10)" ($rc_plain -ge 10) ("got " + $rc_plain)
& $exe @X -y -f $base 2>&1 | Out-Null
$rc_force = $LASTEXITCODE
Check "T14 succeeds with -f (RC = 0)" ($rc_force -eq 0) ("got " + $rc_force)
Check "T14 tree deleted" (-not (Test-Path $base)) "tree still there"
if (Test-Path $base) {
    icacls $base /grant "*S-1-1-0:(OI)(CI)F" | Out-Null
    Remove-Item -LiteralPath $base -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Output "=== T15: locked file + -f -> locker killed, tree deleted ==="
New-TestTree
$sleeper = Start-Locker
Start-Sleep 2
$out = & $exe @X -y -f $base 2>&1
Check "T15 exit code 0" ($LASTEXITCODE -eq 0) ("got " + $LASTEXITCODE)
$unlock = ($out | Select-String "\[unlock\]").Count
Check "T15 [unlock] reported" ($unlock -ge 1) ("output: " + ($out -join " | "))
Check "T15 tree deleted" (-not (Test-Path $base)) "tree still there"
Stop-Locker $sleeper
Remove-Item -LiteralPath $base -Recurse -Force -ErrorAction SilentlyContinue

Write-Output "=== T16: locked file WITHOUT -f -> error reported, exit 10 ==="
New-TestTree
$sleeper = Start-Locker
Start-Sleep 2
$out = & $exe @X -y $base 2>&1
$rc = $LASTEXITCODE
Check "T16 exit code >= 10" ($rc -ge 10) ("got " + $rc)
Stop-Locker $sleeper
Start-Sleep 1
Remove-Item -LiteralPath $base -Recurse -Force -ErrorAction SilentlyContinue

Write-Output "=== T17: --trusted (with --no-elevate) behaves like -f on a locked file ==="
New-TestTree
$sleeper = Start-Locker
Start-Sleep 2
$out = & $exe @X -T --no-elevate -y $base 2>&1
Check "T17 exit code 0" ($LASTEXITCODE -eq 0) ("got " + $LASTEXITCODE)
$unlock = ($out | Select-String "\[unlock\]").Count
Check "T17 [unlock] reported" ($unlock -ge 1) ("output: " + ($out -join " | "))
Check "T17 tree deleted" (-not (Test-Path $base)) "tree still there"
Stop-Locker $sleeper
Remove-Item -LiteralPath $base -Recurse -Force -ErrorAction SilentlyContinue

Write-Output "=== T18: default (no flags) deletes directly, no confirmation ==="
New-TestTree
"n" | & $exe @X $base 2>&1 | Out-Null
Check "T18 exit code 0" ($LASTEXITCODE -eq 0) ("got " + $LASTEXITCODE)
Check "T18 tree deleted without confirmation" (-not (Test-Path $base)) "tree still there"

Write-Output ""
if ($failures -eq 0) { Write-Output "ALL TESTS PASSED" } else { Write-Output ($failures.ToString() + " TEST(S) FAILED") }
exit ($failures -ne 0)
