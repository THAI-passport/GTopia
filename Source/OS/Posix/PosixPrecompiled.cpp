#include "../../Utils/StringUtils.h"
#include "../OSPrecompiled.h"
#include <cerrno>
#include <ctime>
#include <dirent.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <stdlib.h>
#else
#include <sys/random.h>
#endif

string GetDateTimeAsStr()
{
    struct timeval time;
    gettimeofday(&time, NULL);

    time_t nowTime = time.tv_sec;
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&nowTime));

    return string(buf);
}

uint64 GetTick()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

string GetProgramPath()
{
    char buf[1024] = {0};

#if defined(__APPLE__)
    uint32 size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0)
        return "";

    char realPathBuf[1024] = {0};
    if (realpath(buf, realPathBuf) != nullptr)
        return string(dirname(realPathBuf));

    return string(dirname(buf));
#else
    if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) < 0)
        return "";

    return string(dirname(buf));
#endif
}

int32 SleepMS(uint64 ms)
{
    timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;

    return nanosleep(&ts, nullptr);
}

int32 GetRandomBytes(void* pDest, uint32 size)
{
    uint32 offset = 0;
    uint8 attm = 0;

    while (offset < size)
    {
        int32 byteSize = 0;

#if defined(__APPLE__)
        arc4random_buf((uint8*)pDest + offset, size - offset);
        byteSize = size - offset;
#else
        byteSize = getrandom((uint8*)pDest + offset, size - offset, 0);
#endif

        if (byteSize <= 0)
        {
            if (byteSize < 0 && errno == EINTR)
            {
                if (++attm > RANDOM_BYTE_MAX_RETRIES)
                    return -1;

                continue;
            }
            return false;
        }

        offset += byteSize;
    }

    return offset;
}

bool IsFileExists(const string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool IsFolderExists(const string& path)
{
    struct stat buffer;
    return ((stat(path.c_str(), &buffer) == 0) && S_ISDIR(buffer.st_mode));
}

string GetFileExtension(const string& file)
{
    usize index = file.find_last_of('.');
    if (index != string::npos)
    {
        return file.substr(index + 1, file.length());
    }

    return "";
}

uint32 GetSecondsFromMidnight()
{
    time_t now = time(nullptr);
    struct tm time;

    localtime_r(&now, &time);

    return time.tm_hour * 3600 + time.tm_min * 60 + time.tm_sec;
}

string GetLoadAvgString()
{
    double avg[3] = {0.0};
    if (getloadavg(avg, 3) < 0)
    {
        return "";
    }

    return ToString(avg[0]) + " " + ToString(avg[1]) + " " + ToString(avg[2]);
}

bool CreateDir(const string& path)
{
    if (IsFolderExists(path))
        return true;

    return mkdir(path.c_str(), 0755) == 0;
}

void GetTimeLocal(time_t* currentTime, int32* month, int32* day, int32* year, int32* hour, int32* minute, int32* second)
{
    tm timeInfo;
    time_t curTime = currentTime ? *currentTime : std::time(nullptr);

    localtime_r(&currentTime, &timeInfo);

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
    localtime_r(timer, outTime);
}

void GetTimeUTC(tm* outTime, const time_t* timer)
{
    gmtime_r(timer, outTime);
}

uint64 GetLastWriteTimeOfFile(const string& path)
{
    if (path.empty())
        return 0;

    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0)
    {
#ifdef __APPLE__
        return ((uint64)(buffer.st_mtimespec.tv_sec) * 1000000000ULL) + (uint64)(buffer.st_mtimespec.tv_nsec);
#else
        return ((uint64)(buffer.st_mtim.tv_sec) * 1000000000ULL) + (uint64)(buffer.st_mtim.tv_nsec);
#endif
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

        if (!currentDir.empty() && currentDir.back() != '/')
            currentDir += '/';

        DIR* dir = opendir(currentDir.c_str());
        if (!dir)
            continue;

        while (dirent* entry = readdir(dir))
        {
            string nameStr = entry->d_name;
            if (nameStr == "." || nameStr == "..")
                continue;

            string fullPath = currentDir + nameStr;

// https://stackoverflow.com/questions/35215109/struct-dirent-does-not-have-de-type-in-header-file
// this may speed up things
#ifdef _DIRENT_HAVE_D_TYPE
            if (entry->d_type != DT_UNKNOWN)
            {
                if (entry->d_type == DT_DIR)
                {
                    if (recursive)
                        dirsToScan.push_back(fullPath);
                    continue;
                }
                else if (entry->d_type == DT_REG)
                {
                    usize extPos = nameStr.find_last_of('.');
                    if (cleanExt.empty() ||
                        (extPos != string::npos && (nameStr.size() - (extPos + 1)) == cleanExt.size() &&
                         std::memcmp(nameStr.data() + extPos + 1, cleanExt.data(), cleanExt.size()) == 0))
                    {
                        results.push_back(fullPath);
                    }
                    continue;
                }
            }
#endif
            struct stat buffer;
            if (stat(fullPath.c_str(), &buffer) == 0)
            {
                if (S_ISDIR(buffer.st_mode))
                {
                    if (recursive)
                    {
                        dirsToScan.push_back(fullPath);
                    }
                }
                else if (S_ISREG(buffer.st_mode))
                {
                    usize extPos = nameStr.find_last_of('.');
                    if (cleanExt.empty() ||
                        (extPos != string::npos && (nameStr.size() - (extPos + 1)) == cleanExt.size() &&
                         std::memcmp(nameStr.data() + extPos + 1, cleanExt.data(), cleanExt.size()) == 0))
                    {
                        results.push_back(fullPath);
                    }
                }
            }
        }
        closedir(dir);
    }

    return results;
}