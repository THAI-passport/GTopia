#pragma once

#ifdef ENABLE_LUA_SCRIPTING

#include "IO/File.h"
#include "IO/Log.h"
#include "LuaBinder.h"
#include "Precompiled.h"
#include "Utils/StringUtils.h"
#include <chrono>

inline int64 LuaHeapBytes(lua_State* state)
{
    if (!state)
    {
        return 0;
    }
    return (int64)lua_gc(state, LUA_GCCOUNT, 0) * 1024 + lua_gc(state, LUA_GCCOUNTB, 0);
}

struct LuaCallbackEntry
{
    uint32 id;
    int32 luaRef;
    string scriptPath;
    bool dead = false;
};

class LuaCallbackRegistry
{
public:
    uint32 Register(lua_State* state, uint32 eventHash, int funcIndex, const string& scriptPath);
    bool Unregister(lua_State* state, uint32 id);
    uint32 UnregisterScript(lua_State* state, const string& scriptPath, uint32 idFrom = 0, uint32 idTo = 0xFFFFFFFF);
    uint32 CountForScript(const string& scriptPath);
    uint32 PeekNextID() { return m_nextID; }
    std::vector<LuaCallbackEntry>* GetCallbacks(uint32 eventHash);
    void Clear(lua_State* state);

    void BeginDispatch() { m_dispatchDepth++; }
    void EndDispatch();

private:
    struct IdInfo
    {
        uint32 eventHash;
        int32 luaRef;
    };

    uint32 m_nextID = 1;
    uint32 m_dispatchDepth = 0;
    bool m_needsCompaction = false;
    std::unordered_map<uint32, std::vector<LuaCallbackEntry>> m_callbacks;
    std::unordered_map<uint32, IdInfo> m_idInfo;
};

struct LuaProfileStats
{
    uint32 eventHash = 0;
    string path;
    uint64 calls = 0;
    double totalMs = 0.0;
    double maxMs = 0.0;
    uint64 allocatedBytes = 0;
    uint64 slowCalls = 0;
};

enum LuaProfileSort
{
    LUA_PROFILE_SORT_TIME,
    LUA_PROFILE_SORT_AVERAGE,
    LUA_PROFILE_SORT_MAX,
    LUA_PROFILE_SORT_CALLS,
    LUA_PROFILE_SORT_MEMORY
};

class LuaProfiler
{
public:
    void Record(uint32 eventHash, const string& scriptPath, double ms, int64 memDelta);
    string Dump(LuaProfileSort sort = LUA_PROFILE_SORT_TIME, usize topN = 15);
    void Reset();
    void SetEventName(uint32 eventHash, const char* name);

    bool enabled = true;
    double slowCallMs = 10.0;
    int64 memSpikeBytes = 1024 * 1024;

private:
    string EventName(uint32 eventHash);

    std::unordered_map<string, LuaProfileStats> m_stats;
    std::unordered_map<uint32, string> m_eventNames;
};

namespace LuaError
{
int Traceback(lua_State* state);
void Print(lua_State* state, const string& scriptName, const char* label);
} // namespace LuaError

class LuaManager
{
public:
    LuaManager();
    ~LuaManager();

public:
    static LuaManager* GetInstance()
    {
        static LuaManager instance;
        return &instance;
    }

public:
    bool Initialize();
    void Kill();

    bool LoadScript(const string& path);
    uint32 LoadScriptsFromDirectory(const string& dirPath, bool recursive = true, const string& extension = "lua");
    bool UnloadScript(const string& path);
    bool IsLoaded(const string& path) { return m_scripts.find(path) != m_scripts.end(); }

    bool ReloadScript(const string& path);
    bool ReloadByName(const string& name);
    uint32 Reload(const string& target);
    uint32 ReloadAll();

    string DumpProfile(LuaProfileSort sort = LUA_PROFILE_SORT_TIME, usize topN = 15);
    void ResetProfile();
    LuaProfiler& GetProfiler() { return m_profiler; }

    void RegisterFunction(const char* luaName, lua_CFunction func);
    void RegisterCallbackBinding(const char* globalLuaFuncName, uint32 eventHash, const char* eventName = nullptr);

    template <typename T> LuaClass<T> BindClass(const char* className) { return LuaClass<T>(m_L, className); }

    uint32 RegisterCallback(uint32 eventHash, const string& explicitScriptPath = "");
    bool UnregisterCallback(uint32 id);

    template <typename... Args> uint32 InvokeCallback(uint32 eventHash, const Args&... args)
    {
        return ExecuteCallbacks<false>(eventHash, args...);
    }

    template <typename... Args> bool InvokeCallbackHandled(uint32 eventHash, const Args&... args)
    {
        return ExecuteCallbacks<true>(eventHash, args...) > 0;
    }

    template <typename Ret, typename... Args> void DefFunction(const char* name, Ret (*func)(Args...))
    {
        ::DefFunction(m_L, name, func);
    }

    lua_State* GetState() { return m_L; }
    LuaCallbackRegistry& GetCallbackRegistry() { return m_callbackRegistry; }
    string& GetCurrentLoadingScript() { return m_currentScript; }
    uint32 GetScriptCount() { return m_scripts.size(); }

private:
    template <bool CheckHandled, typename... Args> uint32 ExecuteCallbacks(uint32 eventHash, const Args&... args)
    {
        auto* callbacks = m_callbackRegistry.GetCallbacks(eventHash);
        if (!m_L || !callbacks || callbacks->empty())
            return 0;

        int32 numArgs = sizeof...(Args);
        if (!lua_checkstack(m_L, 2 * numArgs + 8))
            return 0;

        int topBefore = lua_gettop(m_L);
        lua_pushcfunction(m_L, LuaError::Traceback);
        int errFunc = lua_gettop(m_L);

        (PushSingleArg(args), ...);

        m_callbackRegistry.BeginDispatch();
        usize count = callbacks->size();
        uint32 handledCount = 0;

        for (usize index = 0; index < count; ++index)
        {
            if ((*callbacks)[index].dead)
                continue;

            lua_rawgeti(m_L, LUA_REGISTRYINDEX, (*callbacks)[index].luaRef);
            if (!lua_isfunction(m_L, -1))
            {
                lua_pop(m_L, 1);
                continue;
            }

            for (int32 argIndex = 0; argIndex < numArgs; ++argIndex)
            {
                lua_pushvalue(m_L, (errFunc + 1) + argIndex);
            }

            bool profiling = m_profiler.enabled;

            std::chrono::steady_clock::time_point startTime;
            int64 memStart = 0;
            if (profiling)
            {
                startTime = std::chrono::steady_clock::now();
                memStart = LuaHeapBytes(m_L);
            }

            int32 status = lua_pcall(m_L, numArgs, 1, errFunc);

            if (profiling)
            {
                double ms =
                    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
                m_profiler.Record(eventHash, (*callbacks)[index].scriptPath, ms, LuaHeapBytes(m_L) - memStart);
            }

            if (status == LUA_OK)
            {
                handledCount++;
                bool wasHandled = lua_toboolean(m_L, -1) != 0;
                lua_pop(m_L, 1);

                if (CheckHandled && wasHandled)
                {
                    lua_settop(m_L, topBefore);
                    m_callbackRegistry.EndDispatch();
                    return handledCount;
                }
            }
            else
            {
                LuaError::Print(m_L, (*callbacks)[index].scriptPath, "ERROR");
                lua_pop(m_L, 1);
            }
        }

        lua_settop(m_L, topBefore);
        m_callbackRegistry.EndDispatch();
        return CheckHandled ? 0 : handledCount;
    }

    template <typename T> void PushSingleArg(T val) { PushArg(m_L, val); }

    bool LoadScriptMemory(const string& path, char* buffer, usize size);
    string FindLoadedScript(const string& query);
    string FindOnDisk(const string& query);

    lua_State* m_L;
    string m_currentScript;
    string m_rootDir;
    std::unordered_map<string, bool> m_scripts;

    LuaCallbackRegistry m_callbackRegistry;
    LuaProfiler m_profiler;
};

LuaManager* GetLuaManager();

#endif