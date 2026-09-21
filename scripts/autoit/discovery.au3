#include <MsgBoxConstants.au3>

Local $computer = @ComputerName
Local $user = @UserName

MsgBox( _
    $MB_ICONINFORMATION, _
    "WinAPI Lab - AutoIT", _
    "Computer: " & $computer & @CRLF & _
    "User: " & $user _
)
