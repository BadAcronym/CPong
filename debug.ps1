Invoke-Expression ".\run -dontrun"
if(-Not(Test-Path ".\bin\Win64_Debug\CPong.exe"))
{
    Write-Host "no executable to debug."
    return;
}

&raddbg
