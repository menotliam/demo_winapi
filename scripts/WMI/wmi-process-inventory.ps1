Write-Host "=== WMI/CIM Process Inventory Lab ==="
Get-CimInstance Win32_Process |
    Select-Object Name, ProcessId, ParentProcessId |
    Sort-Object Name |
    Select-Object -First 25
