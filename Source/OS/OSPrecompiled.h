#pragma once
#include <cstddef>
#include <stdint.h>
#include <string>
#include <vector>

#define RANDOM_BYTE_MAX_RETRIES 2

typedef uint64_t uint64;
typedef int64_t int64;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint8_t uint8;
typedef int8_t int8;
typedef size_t usize;
typedef std::string string;

string GetDateTimeAsStr();
uint64 GetTick();
string GetProgramPath();
int32 SleepMS(uint64 ms);
int32 GetRandomBytes(void* pDest, uint32 size);
bool IsFileExists(const string& path);
bool IsFolderExists(const string& path);
string GetFileExtension(const string& file);
uint32 GetSecondsFromMidnight();
string GetLoadAvgString();
string GetFileNameFromPath(const string& path);
bool CreateDir(const string& path);
void GetTimeLocal(time_t* currentTime, int32* month, int32* day, int32* year, int32* hour, int32* minute,
                  int32* second);
void GetTimeLocal(tm* outTime, time_t* timer);
void GetTimeUTC(tm* outTime, const time_t* timer);
uint64 GetLastWriteTimeOfFile(const string& path); // returns ns
std::vector<string> ScanDirectoryFiles(const string& dirPath, bool recursive = true, const string& extension = "");

#ifdef _WIN32
std::wstring UTF8ToUTF16(const string& str);
string UTF16ToUTF8(const std::wstring& wstr);
#endif