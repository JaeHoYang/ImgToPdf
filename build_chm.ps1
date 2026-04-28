$doxygenExe = "C:\Tools\doxygen\doxygen.exe"
$hhcExe     = "C:\Program Files (x86)\HTML Help Workshop\hhc.exe"
$hhpFile    = "docs\html\index.hhp"
$hhcFile    = "docs\html\index.hhc"
$hhkFile    = "docs\html\index.hhk"
$doxyCss    = "docs\html\doxygen.css"

Write-Host "[1/4] Running doxygen..."
& $doxygenExe Doxyfile
Write-Host "[1/4] Done"

Write-Host "[2/4] Patching doxygen.css for font size..."
if (Test-Path $doxyCss) {
    $patch = @"

/* ---- font-size override (appended by build_chm.ps1) ---- */
body, div, p, td, th, li, dt, dd, span, a, label {
    font-size: 17px !important;
    line-height: 1.75 !important;
}
h1 { font-size: 26px !important; }
h2 { font-size: 23px !important; }
h3 { font-size: 21px !important; }
h4 { font-size: 19px !important; }
h5, h6 { font-size: 17px !important; }
.memname, .memitem, .memdoc, .memtitle, .permalink { font-size: 16px !important; }
code, pre, .fragment, .line, .lineno { font-size: 15px !important; line-height: 1.6 !important; }
.tabs, .tabs2, .tabs3, .navpath, .navtab, .qindex, .summary { font-size: 15px !important; }
.textblock, .mdescLeft, .mdescRight { font-size: 17px !important; line-height: 1.8 !important; }
table.params td, table.retval td, .fieldtable td, .fieldtable th { font-size: 16px !important; padding: 5px 8px !important; }
.mlabel, .icon, .arrow, .levels { font-size: 13px !important; }
"@
    Add-Content -Path $doxyCss -Value $patch -Encoding UTF8
    Write-Host "  Patched: $doxyCss"
} else {
    Write-Host "  WARNING: $doxyCss not found"
}
Write-Host "[2/4] Done"

Write-Host "[3/4] Converting .hhc/.hhk UTF-8 -> CP949..."
$enc949 = [System.Text.Encoding]::GetEncoding(949)
foreach ($f in @($hhcFile, $hhkFile)) {
    if (Test-Path $f) {
        $text = [System.IO.File]::ReadAllText($f)
        [System.IO.File]::WriteAllText($f, $text, $enc949)
        Write-Host "  Converted: $f"
    }
}
Write-Host "[3/4] Done"

Write-Host "[4/4] Running hhc.exe..."
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
