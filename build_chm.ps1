$doxygenExe = "C:\Tools\doxygen\doxygen.exe"
$hhcExe     = "C:\Program Files (x86)\HTML Help Workshop\hhc.exe"
$hhpFile    = "docs\html\index.hhp"
$hhcFile    = "docs\html\index.hhc"
$hhkFile    = "docs\html\index.hhk"

Write-Host "[1/3] Running doxygen..."
& $doxygenExe Doxyfile
Write-Host "[1/3] Done"

Write-Host "[2/3] Converting .hhc/.hhk UTF-8 -> CP949..."
$enc949 = [System.Text.Encoding]::GetEncoding(949)
foreach ($f in @($hhcFile, $hhkFile)) {
    if (Test-Path $f) {
        $text = [System.IO.File]::ReadAllText($f)
        [System.IO.File]::WriteAllText($f, $text, $enc949)
        Write-Host "  Converted: $f"
    }
}
Write-Host "[2/3] Done"

Write-Host "[3/3] Running hhc.exe..."
Stop-Process -Name "hh" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 500
& $hhcExe $hhpFile

$chmPath = "docs\ImgToPdf.chm"
if (Test-Path $chmPath) {
    $size = [math]::Round((Get-Item $chmPath).Length / 1MB, 1)
    Write-Host "CHM ready: $(Resolve-Path $chmPath) ($size MB)"
} else {
    Write-Host "ERROR: CHM not found"
}
