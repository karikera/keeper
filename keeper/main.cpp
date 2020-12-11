
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <iostream>
#include <sstream>

using namespace std;

HANDLE findProcessByName(PCWSTR name) noexcept
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    memset(&process, 0, sizeof(process));
    process.dwSize = sizeof(process);

    if (Process32FirstW(snapshot, &process))
    {
        do
        {
            if (wcscmp(process.szExeFile, name) == 0)
            {
                DWORD pid = process.th32ProcessID;
                CloseHandle(snapshot);
                return OpenProcess(PROCESS_ALL_ACCESS, false, pid);
            }
        } while (Process32NextW(snapshot, &process));
    }

    CloseHandle(snapshot);
    return nullptr;
}

wstring readParameter(LPWSTR * pstr) noexcept
{
    wstringstream out;

    wchar_t chr;
    LPWSTR str = *pstr;
    for (;; str++)
    {
    _out_quot:;
        chr = *str;

        switch (chr)
        {
        case '\"':
            str++;
            for (;; str++)
            {
                chr = *str;
                switch (chr)
                {
                case '\0': goto _fin;
                case '"':
                    if (str[-1] == '\\')
                    {
                        out << '"';
                        continue;
                    }
                    str++;
                    if (*str == '"')
                    {
                        out << '"';
                        continue;
                    }
                    goto _out_quot;
                default:
                    out << chr;
                    break;
                }
            }
            break;
        case '\0': goto _fin;
        case ' ': str++; goto _fin;
        default:
            out << chr;
            break;
        }
    }

_fin:
    *pstr = str;
    return move(out.str());
}

int wmain(int argn, wchar_t** argv)
{
    if (argn < 2)
    {
        cerr << "Too few arguments" << endl;
        cerr << "keeper [target] [executer]" << endl;
        return EINVAL;
    }
    wchar_t* findTarget = argv[1];

    LPWSTR cmdline = GetCommandLineW();
    readParameter(&cmdline);
    readParameter(&cmdline);
    wstring runTarget = readParameter(&cmdline);

    for (;;)
    {
        HANDLE handle = findProcessByName(findTarget);
        if (handle == nullptr)
        {
            cout << "Rerun program" << endl;
            HINSTANCE instance = ShellExecuteW(nullptr, L"open", runTarget.c_str(), cmdline, nullptr, SW_SHOW);
            if ((uintptr_t)instance <= 32)
            {
                cerr << "Cannot run program" << endl;
            }
            Sleep(1000);
        }
        else
        {
            WaitForSingleObject(handle, INFINITE);
            CloseHandle(handle);
            cout << "Program closed" << endl;
        }
    }
}

