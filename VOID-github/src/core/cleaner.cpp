#include "cleaner.hpp"
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <vector>

namespace cleaner
{
    namespace
    {
        std::wstring Env(const wchar_t* var)
        {
            wchar_t buf[MAX_PATH];
            DWORD n = GetEnvironmentVariableW(var, buf, MAX_PATH);
            return n > 0 ? std::wstring(buf, n) : L"";
        }

        void ScanDir(const std::wstring& path, double& mb, int& count, bool recurse = true)
        {
            WIN32_FIND_DATAW fd;
            HANDLE h = FindFirstFileW((path + L"\\*").c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE) return;
            do {
                if (fd.cFileName[0] == L'.' && (fd.cFileName[1] == 0 || (fd.cFileName[1] == L'.' && fd.cFileName[2] == 0)))
                    continue;
                std::wstring full = path + L"\\" + fd.cFileName;
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (recurse) ScanDir(full, mb, count, true);
                }
                else
                {
                    ULARGE_INTEGER sz;
                    sz.LowPart  = fd.nFileSizeLow;
                    sz.HighPart = fd.nFileSizeHigh;
                    mb += (double)sz.QuadPart / (1024.0 * 1024.0);
                    count++;
                }
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        }

        void ScanPattern(const std::wstring& dir, const wchar_t* pattern, double& mb, int& count)
        {
            WIN32_FIND_DATAW fd;
            HANDLE h = FindFirstFileW((dir + L"\\" + pattern).c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE) return;
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    ULARGE_INTEGER sz;
                    sz.LowPart  = fd.nFileSizeLow;
                    sz.HighPart = fd.nFileSizeHigh;
                    mb += (double)sz.QuadPart / (1024.0 * 1024.0);
                    count++;
                }
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        }

        int CleanDir(const std::wstring& path, bool recurse = true)
        {
            int deleted = 0;
            WIN32_FIND_DATAW fd;
            HANDLE h = FindFirstFileW((path + L"\\*").c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE) return 0;
            do {
                if (fd.cFileName[0] == L'.' && (fd.cFileName[1] == 0 || (fd.cFileName[1] == L'.' && fd.cFileName[2] == 0)))
                    continue;
                std::wstring full = path + L"\\" + fd.cFileName;
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (recurse) deleted += CleanDir(full, true);
                    RemoveDirectoryW(full.c_str());
                }
                else
                {
                    SetFileAttributesW(full.c_str(), FILE_ATTRIBUTE_NORMAL);
                    if (DeleteFileW(full.c_str())) deleted++;
                }
            } while (FindNextFileW(h, &fd));
            FindClose(h);
            return deleted;
        }

        int CleanPattern(const std::wstring& dir, const wchar_t* pattern)
        {
            int deleted = 0;
            WIN32_FIND_DATAW fd;
            HANDLE h = FindFirstFileW((dir + L"\\" + pattern).c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE) return 0;
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    std::wstring full = dir + L"\\" + fd.cFileName;
                    SetFileAttributesW(full.c_str(), FILE_ATTRIBUTE_NORMAL);
                    if (DeleteFileW(full.c_str())) deleted++;
                }
            } while (FindNextFileW(h, &fd));
            FindClose(h);
            return deleted;
        }
    }

    ScanResult Scan(int cat)
    {
        ScanResult r;
        std::wstring temp  = Env(L"TEMP");
        std::wstring local = Env(L"LOCALAPPDATA");
        std::wstring win   = Env(L"WINDIR");

        switch (cat)
        {
        case 0:
            if (!temp.empty())  ScanDir(temp, r.sizeMB, r.fileCount);
            if (!win.empty())   ScanDir(win + L"\\Temp", r.sizeMB, r.fileCount);
            break;
        case 1:
            if (!local.empty())
            {
                ScanDir(local + L"\\Google\\Chrome\\User Data\\Default\\Cache", r.sizeMB, r.fileCount);
                ScanDir(local + L"\\Google\\Chrome\\User Data\\Default\\Code Cache", r.sizeMB, r.fileCount);
                ScanDir(local + L"\\Microsoft\\Edge\\User Data\\Default\\Cache", r.sizeMB, r.fileCount);
                ScanDir(local + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache", r.sizeMB, r.fileCount);
                ScanDir(local + L"\\Mozilla\\Firefox\\Profiles", r.sizeMB, r.fileCount);
                ScanDir(local + L"\\Opera Software\\Opera Stable\\Cache", r.sizeMB, r.fileCount);
                ScanDir(local + L"\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Cache", r.sizeMB, r.fileCount);
            }
            break;
        case 2:
            if (!win.empty()) ScanDir(win + L"\\SoftwareDistribution\\Download", r.sizeMB, r.fileCount);
            break;
        case 3:
        {
            SHQUERYRBINFO info = { sizeof(info) };
            if (SUCCEEDED(SHQueryRecycleBinW(nullptr, &info)))
            {
                r.sizeMB    = (double)info.i64Size / (1024.0 * 1024.0);
                r.fileCount = (int)info.i64NumItems;
            }
            break;
        }
        case 4:
            if (!win.empty()) ScanDir(win + L"\\Prefetch", r.sizeMB, r.fileCount);
            break;
        case 5:
            if (!win.empty())
            {
                ScanDir(win + L"\\Logs", r.sizeMB, r.fileCount);
                ScanDir(win + L"\\System32\\LogFiles", r.sizeMB, r.fileCount);
                ScanPattern(win + L"\\System32\\winevt\\Logs", L"*.evtx", r.sizeMB, r.fileCount);
            }
            break;
        case 6:
            if (!local.empty())
                ScanPattern(local + L"\\Microsoft\\Windows\\Explorer", L"thumbcache_*.db", r.sizeMB, r.fileCount);
            break;
        case 7:
            if (!local.empty()) ScanDir(local + L"\\CrashDumps", r.sizeMB, r.fileCount);
            if (!win.empty())
            {
                ScanDir(win + L"\\Minidump", r.sizeMB, r.fileCount);
                ScanPattern(win, L"MEMORY.DMP", r.sizeMB, r.fileCount);
            }
            break;
        case 8:
            if (!local.empty())
            {
                ScanDir(local + L"\\D3DSCache", r.sizeMB, r.fileCount);
                ScanDir(local + L"\\NVIDIA\\DXCache", r.sizeMB, r.fileCount);
                ScanDir(local + L"\\NVIDIA\\GLCache", r.sizeMB, r.fileCount);
                ScanDir(local + L"\\AMD\\DXCache", r.sizeMB, r.fileCount);
            }
            break;
        case 9:
            if (!win.empty()) ScanDir(win + L"\\SoftwareDistribution\\DeliveryOptimization", r.sizeMB, r.fileCount);
            break;
        }
        return r;
    }

    double Clean(int cat)
    {
        double before = Scan(cat).sizeMB;

        std::wstring temp  = Env(L"TEMP");
        std::wstring local = Env(L"LOCALAPPDATA");
        std::wstring win   = Env(L"WINDIR");

        switch (cat)
        {
        case 0:
            if (!temp.empty()) CleanDir(temp);
            if (!win.empty())  CleanDir(win + L"\\Temp");
            break;
        case 1:
            if (!local.empty())
            {
                CleanDir(local + L"\\Google\\Chrome\\User Data\\Default\\Cache");
                CleanDir(local + L"\\Google\\Chrome\\User Data\\Default\\Code Cache");
                CleanDir(local + L"\\Microsoft\\Edge\\User Data\\Default\\Cache");
                CleanDir(local + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache");
                CleanDir(local + L"\\Mozilla\\Firefox\\Profiles");
                CleanDir(local + L"\\Opera Software\\Opera Stable\\Cache");
                CleanDir(local + L"\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Cache");
            }
            break;
        case 2:
            if (!win.empty()) CleanDir(win + L"\\SoftwareDistribution\\Download");
            break;
        case 3:
            SHEmptyRecycleBinW(nullptr, nullptr, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);
            return before;
        case 4:
            if (!win.empty()) CleanDir(win + L"\\Prefetch");
            break;
        case 5:
            if (!win.empty())
            {
                CleanDir(win + L"\\Logs");
                CleanDir(win + L"\\System32\\LogFiles");
            }
            break;
        case 6:
            if (!local.empty())
                CleanPattern(local + L"\\Microsoft\\Windows\\Explorer", L"thumbcache_*.db");
            break;
        case 7:
            if (!local.empty()) CleanDir(local + L"\\CrashDumps");
            if (!win.empty())
            {
                CleanDir(win + L"\\Minidump");
                CleanPattern(win, L"MEMORY.DMP");
            }
            break;
        case 8:
            if (!local.empty())
            {
                CleanDir(local + L"\\D3DSCache");
                CleanDir(local + L"\\NVIDIA\\DXCache");
                CleanDir(local + L"\\NVIDIA\\GLCache");
                CleanDir(local + L"\\AMD\\DXCache");
            }
            break;
        case 9:
            if (!win.empty()) CleanDir(win + L"\\SoftwareDistribution\\DeliveryOptimization");
            break;
        }

        double after = Scan(cat).sizeMB;
        return before > after ? before - after : 0.0;
    }
}
