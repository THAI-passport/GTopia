#include "../OSPrecompiled.h"

// clang-format off
#include <ctime>
#include <windows.h>
#include <wincrypt.h>
// clang-format on

string GetDateTimeAsStr()
{
    time_t now = time(nullptr);

    struct tm timeInfo;
    char buf[64];
    timeInfo = *localtime(&now);

    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeInfo);
    return string(buf);
}

uint64 GetTick()
{
    return GetTickCount64();
}

string GetProgramPath()
{
    wchar_t buf[1024] = {0};

    uint32 len = GetModuleFileNameW(NULL, buf, 1024);
    if (len == 0)
        return "";

    string path = UTF16ToUTF8(std::wstring(buf, len));

    usize pos = path.find_last_of("\\/");
    if (pos != string::npos)
    {
        path = path.substr(0, pos);
    }

    return path;
}

int32 SleepMS(uint64 ms)
{
    ::Sleep(ms);
    return 0;
}

int32 GetRandomBytes(void* pDest, uint32 size)
{
    HCRYPTPROV hProv = 0;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        return -1;
    }

    if (!CryptGenRandom(hProv, size, (BYTE*)pDest))
    {
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    CryptReleaseContext(hProv, 0);
    return size; // umm it says always return?
}

bool IsFileExists(const string& path)
{
    if (path.empty())
        return false;

    DWORD attr = GetFileAttributesW(UTF8ToUTF16(path).c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY))
        return false;

    return true;
}

bool IsFolderExists(const string& path)
{
    if (path.empty())
        return false;

    DWORD attr = GetFileAttributesW(UTF8ToUTF16(path).c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
        return false;

    return true;
}

string GetFileExtension(const string& file)
{
    usize index = file.find_last_of('.');
    if (index != string::npos)
        return file.substr(index + 1, file.length());

    return "";
}

uint32 GetSecondsFromMidnight()
{
    time_t now = time(nullptr);
    struct tm time;

    localtime_s(&time, &now);

    return time.tm_hour * 3600 + time.tm_min * 60 + time.tm_sec;
}

string GetLoadAvgString()
{
    return "";
}

uint64 GetLastWriteTimeOfFile(const string& path)
{
    if (path.empty())
        return 0;

    std::wstring wPath = UTF8ToUTF16(path);
    if (wPath.empty())
        return 0;

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExW(wPath.c_str(), GetFileExInfoStandard, &data))
    {
        ULARGE_INTEGER uli;
        uli.LowPart = data.ftLastWriteTime.dwLowDateTime;
        uli.HighPart = data.ftLastWriteTime.dwHighDateTime;

        return uli.QuadPart;
    }
    return 0;
}

std::vector<string> ScanDirectoryFiles(const string& dirPath, bool recursive, const string& extension)
{
    std::vector<string> results;
    if (dirPath.empty())
        return results;

    std::vector<string> dirsToScan = {dirPath};
    string cleanExt = (!extension.empty() && extension.front() == '.') ? extension.substr(1) : extension;

    while (!dirsToScan.empty())
    {
        string currentDir = dirsToScan.back();
        dirsToScan.pop_back();

        if (!currentDir.empty() && currentDir.back() != '/' && currentDir.back() != '\\')
            currentDir += '/';

        WIN32_FIND_DATAW findData;
        HANDLE hFind =
            FindFirstFileExW(UTF8ToUTF16(currentDir + "*").c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch,
                             nullptr, FIND_FIRST_EX_LARGE_FETCH); // test large fetch, the performance
                                                                  // also this might be better and non ExW

        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        // already fetched first file
        do
        {
            if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0)
                continue;

            string nameStr = UTF16ToUTF8(findData.cFileName);
            string fullPath = currentDir + nameStr;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (recursive)
                    dirsToScan.push_back(fullPath);
            }
            else
            {
                usize extPos = nameStr.find_last_of('.');
                if (cleanExt.empty() ||
                    (extPos != string::npos && (nameStr.size() - (extPos + 1)) == cleanExt.size() &&
                     memcmp(nameStr.data() + extPos + 1, cleanExt.data(), cleanExt.size()) == 0)) // lets go with memcmp
                {
                    results.push_back(fullPath);
                }
            }
        } while (FindNextFileW(hFind, &findData));

        FindClose(hFind);
    }

    return results;
}

std::wstring UTF8ToUTF16(const string& str)
{
    if (str.empty())
        return std::wstring();

    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (size <= 0)
        return std::wstring();

    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);

    return result;
}

string UTF16ToUTF8(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();

    int targetLen = ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (targetLen <= 0)
        return std::string();

    string str(targetLen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &str[0], targetLen, nullptr, nullptr);

    return str;
}

bool CreateDir(const string& path)
{
    if (IsFolderExists(path))
        return true;

    std::wstring wpath = UTF8ToUTF16(path);
    return CreateDirectoryW(wpath.c_str(), nullptr) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

void GetTimeLocal(time_t* currentTime, int32* month, int32* day, int32* year, int32* hour, int32* minute, int32* second)
{
    tm timeInfo;
    time_t curTime = currentTime ? *currentTime : time(nullptr);

    localtime_s(&timeInfo, &curTime);

    if (month)
        *month = timeInfo.tm_mon + 1;
    if (day)
        *day = timeInfo.tm_mday;
    if (year)
        *year = timeInfo.tm_year + 1900;
    if (hour)
        *hour = timeInfo.tm_hour;
    if (minute)
        *minute = timeInfo.tm_min;
    if (second)
        *second = timeInfo.tm_sec;
}

void GetTimeLocal(tm* outTime, time_t* timer)
{
    localtime_s(outTime, timer);
}

void GetTimeUTC(tm* outTime, const time_t* timer)
{
    gmtime_s(outTime, timer);
}