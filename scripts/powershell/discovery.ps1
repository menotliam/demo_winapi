Write-Host "=== PowerShell Discovery Lab ==="
Write-Host "USER: $env:USERNAME"
Write-Host "COMPUTER: $env:COMPUTERNAME"
Write-Host "PROCESS SAMPLE:"
Get-Process | Select-Object -First 10 Name, Id | Format-Table | Out-Host
Write-Host "LAB FILES:"
Get-ChildItem "C:\Users\lam\Downloads\WinAPI-Lab-Safe-SOC-Demo\WinAPI-Lab\documents" -Recurse |
    Select-Object FullName, Length | Format-Table | Out-Host
