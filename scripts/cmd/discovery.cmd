@echo off
setlocal

echo === CMD Discovery Lab ===
echo USER:
whoami

echo COMPUTER:
hostname

echo PROCESS SAMPLE:
tasklist | findstr /i "explorer cmd powershell"

echo LAB FILES:
dir /s /b C:\Users\lam\Downloads\WinAPI-Lab-Safe-SOC-Demo\WinAPI-Lab\documents
endlocal
