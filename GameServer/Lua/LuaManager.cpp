#ifdef ENABLE_LUA_SCRIPTING

#include "LuaManager.h"
#include "Math/Math.h"

#include "Bindings/CommandBinding.h"
#include "Bindings/DialogBinding.h"
#include "Bindings/DiscordBinding.h"
#include "Bindings/ItemBinding.h"
#include "Bindings/MathBinding.h"
#include "Bindings/PlayerBinding.h"
#include "Bindings/WorldBinding.h"

int LuaError::Traceback(lua_State* state)
{
    const char* msg = lua_tostring(state, 1);
    if (!msg)
    {
        if (luaL_callmeta(state, 1, "__tostring") && lua_type(state, -1) == LUA_TSTRING)
        {
            msg = lua_tostring(state, -1);
        }
        else
        {
            msg = lua_pushfstring(state, "(error object is a %s value)", luaL_typename(state, 1));
        }
    }
    luaL_traceback(state, state, msg, 1);
    return 1;
}

void LuaError::Print(lua_State* state, const string& scriptName, const char* label)
{
    string error = lua_isstring(state, -1) ? lua_tostring(state, -1) : "Unknown error";
    string name = scriptName.empty() ? "(global)" : scriptName;

    LOGGER_LOG_ERROR("[Lua] %s in %s", label, name.c_str());

    usize pos = 0;
    while (pos < error.size())
    {
        usize newline = error.find('\n', pos);
        if (newline == string::npos)
        {
            newline = error.size();
        }

        string line = error.substr(pos, newline - pos);
        pos = newline + 1;

        while (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.empty())
            continue;

        LOGGER_LOG_ERROR("[Lua] %s", line.c_str());
    }
}

void LuaProfiler::SetEventName(uint32 eventHash, const char* name)
{
    if (name && name[0] != '\0')
    {
        m_eventNames[eventHash] = name;
    }
}

string LuaProfiler::EventName(uint32 eventHash)
{
    if (eventHash == 0)
        return "(script load)";

    auto nameIt = m_eventNames.find(eventHash);
    if (nameIt != m_eventNames.end())
        return nameIt->second;

    char buf[16];
    snprintf(buf, sizeof(buf), "0x%08X", eventHash);
    return buf;
}

void LuaProfiler::Record(uint32 eventHash, const string& scriptPath, double ms, int64 memDelta)
{
    if (!enabled)
        return;

    string path = scriptPath.empty() ? "global" : scriptPath;
    LuaProfileStats& stat = m_stats[path + "|" + ToString(eventHash)];

    if (stat.calls == 0)
    {
        stat.eventHash = eventHash;
        stat.path = path;
    }

    stat.calls++;
    stat.totalMs += ms;
    stat.maxMs = Max(stat.maxMs, ms);
    if (memDelta > 0)
    {
        stat.allocatedBytes += memDelta;
    }
    if (ms >= slowCallMs)
    {
        stat.slowCalls++;
    }

    string where = EventName(eventHash) + " @ " + path;
    if (ms >= slowCallMs)
    {
        LOGGER_LOG_WARN("[LuaProfiler] SLOW CALL %s took %.2f ms (limit %.2f ms)", where.c_str(), ms, slowCallMs);
    }
    if (memDelta >= memSpikeBytes)
    {
        LOGGER_LOG_WARN("[LuaProfiler] MEM SPIKE %s allocated %.2f KB in one call", where.c_str(), memDelta / 1024.0);
    }
}

void LuaProfiler::Reset()
{
    m_stats.clear();
}

string LuaProfiler::Dump(LuaProfileSort sort, usize topN)
{
    if (m_stats.empty())
        return "No samples recorded yet.\n";

    std::vector<LuaProfileStats*> rows;
    double totalMs = 0.0;
    for (auto& pair : m_stats)
    {
        rows.push_back(&pair.second);
        totalMs += pair.second.totalMs;
    }

    auto metric = [sort](LuaProfileStats& stat) -> double
    {
        switch (sort)
        {
            case LUA_PROFILE_SORT_AVERAGE:
                return stat.calls ? stat.totalMs / stat.calls : 0.0;
            case LUA_PROFILE_SORT_MAX:
                return stat.maxMs;
            case LUA_PROFILE_SORT_CALLS:
                return stat.calls;
            case LUA_PROFILE_SORT_MEMORY:
                return stat.allocatedBytes;
            default:
                return stat.totalMs;
        }
    };
    std::sort(rows.begin(), rows.end(),
              [&](LuaProfileStats* left, LuaProfileStats* right) { return metric(*left) > metric(*right); });

    char line[256];
    string out;

    snprintf(line, sizeof(line), "---------------- LUA PROFILE (total %.2f ms) ----------------\n", totalMs);
    out += line;
    snprintf(line, sizeof(line), "%-3s %-30s %-20s %8s %10s %10s %10s %6s\n", "#", "Script", "Event", "Calls", "Total",
             "Avg", "Max", "CPU%");
    out += line;

    usize shown = Min(topN, rows.size());
    for (usize index = 0; index < shown; ++index)
    {
        LuaProfileStats& stat = *rows[index];

        double avg = stat.calls ? stat.totalMs / stat.calls : 0.0;
        double share = totalMs > 0.0 ? 100.0 * stat.totalMs / totalMs : 0.0;

        string flags;
        if (stat.slowCalls)
        {
            flags += " SLOW";
        }
        if (stat.calls && stat.allocatedBytes / stat.calls >= 64 * 1024)
        {
            flags += " MEM";
        }

        snprintf(line, sizeof(line), "%-3u %-30s %-20s %8llu %8.2fms %8.2fms %8.2fms %5.1f%%%s\n", index + 1,
                 stat.path.c_str(), EventName(stat.eventHash).c_str(), stat.calls, stat.totalMs, avg, stat.maxMs, share,
                 flags.c_str());
        out += line;
    }

    if (rows.size() > shown)
    {
        snprintf(line, sizeof(line), "... and %u more entries\n", (rows.size() - shown));
        out += line;
    }

    return out;
}

uint32 LuaCallbackRegistry::Register(lua_State* state, uint32 eventHash, int funcIndex, const string& scriptPath)
{
    lua_pushvalue(state, funcIndex);
    int ref = luaL_ref(state, LUA_REGISTRYINDEX);

    uint32 id = m_nextID++;
    m_callbacks[eventHash].push_back({id, ref, scriptPath, false});
    m_idInfo[id] = {eventHash, ref};
    return id;
}

bool LuaCallbackRegistry::Unregister(lua_State* state, uint32 id)
{
    auto idIt = m_idInfo.find(id);
    if (idIt == m_idInfo.end())
        return false;

    uint32 eventHash = idIt->second.eventHash;
    luaL_unref(state, LUA_REGISTRYINDEX, idIt->second.luaRef);
    m_idInfo.erase(idIt);

    auto vecIt = m_callbacks.find(eventHash);
    if (vecIt == m_callbacks.end())
        return true;

    auto& vec = vecIt->second;
    auto found = std::find_if(vec.begin(), vec.end(), [id](LuaCallbackEntry& callback) { return callback.id == id; });
    if (found == vec.end())
        return true;

    if (m_dispatchDepth > 0)
    {
        found->dead = true;
        found->luaRef = LUA_NOREF;
        m_needsCompaction = true;
    }
    else
    {
        vec.erase(found);
        if (vec.empty())
        {
            m_callbacks.erase(vecIt);
        }
    }
    return true;
}

uint32 LuaCallbackRegistry::UnregisterScript(lua_State* state, const string& scriptPath, uint32 idFrom, uint32 idTo)
{
    if (scriptPath.empty())
        return 0;

    std::vector<uint32> ids;
    for (auto& pair : m_callbacks)
    {
        for (auto& entry : pair.second)
        {
            if (!entry.dead && entry.id >= idFrom && entry.id < idTo && entry.scriptPath == scriptPath)
            {
                ids.push_back(entry.id);
            }
        }
    }

    uint32 count = 0;
    for (uint32 id : ids)
    {
        if (Unregister(state, id))
        {
            count++;
        }
    }
    return count;
}

uint32 LuaCallbackRegistry::CountForScript(const string& scriptPath)
{
    uint32 count = 0;
    for (auto& pair : m_callbacks)
    {
        for (auto& entry : pair.second)
        {
            if (!entry.dead && entry.scriptPath == scriptPath)
            {
                count++;
            }
        }
    }
    return count;
}

std::vector<LuaCallbackEntry>* LuaCallbackRegistry::GetCallbacks(uint32 eventHash)
{
    auto callbackIt = m_callbacks.find(eventHash);
    return callbackIt == m_callbacks.end() ? nullptr : &callbackIt->second;
}

void LuaCallbackRegistry::Clear(lua_State* state)
{
    for (auto& pair : m_idInfo)
    {
        luaL_unref(state, LUA_REGISTRYINDEX, pair.second.luaRef);
    }

    m_callbacks.clear();
    m_idInfo.clear();
    m_needsCompaction = false;
}

void LuaCallbackRegistry::EndDispatch()
{
    if (m_dispatchDepth == 0)
        return;

    m_dispatchDepth--;
    if (m_dispatchDepth > 0 || !m_needsCompaction)
        return;

    for (auto callbackIt = m_callbacks.begin(); callbackIt != m_callbacks.end();)
    {
        auto& vec = callbackIt->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(), [](LuaCallbackEntry& entry) { return entry.dead; }),
                  vec.end());

        if (vec.empty())
        {
            callbackIt = m_callbacks.erase(callbackIt);
        }
        else
        {
            ++callbackIt;
        }
    }
    m_needsCompaction = false;
}

LuaManager::LuaManager() : m_L(nullptr) {}

LuaManager::~LuaManager()
{
    Kill();
}

bool LuaManager::Initialize()
{
    if (m_L)
    {
        Kill();
    }

    m_L = luaL_newstate();
    if (!m_L)
    {
        LOGGER_LOG_ERROR("[Lua] Failed to create Lua state!");
        return false;
    }

    luaL_openlibs(m_L);
    m_profiler.Reset();

    RegisterCallbackBinding("onTileWrenchCallback", "OnTileWrench"_hash, "OnTileWrench");
    RegisterCallbackBinding("onConsumeConsumableCallback", "OnConsumeConsumable"_hash, "OnConsumeConsumable");
    RegisterCallbackBinding("onPlayerEnterWorldCallback", "OnPlayerEnterWorld"_hash, "OnPlayerEnterWorld");
    RegisterCallbackBinding("onPlayerLeaveWorld", "OnPlayerLeaveWorld"_hash, "OnPlayerLeaveWorld");

    RegisterPlayerBinding();
    RegisterWorldBinding();
    RegisterCommandBinding();
    RegisterDialogBinding();
    RegisterItemBinding();
    RegisterDiscordBinding();
    RegisterMathBinding();

    return true;
}

void LuaManager::Kill()
{
    if (!m_L)
    {
        return;
    }

    m_callbackRegistry.Clear(m_L);
    m_scripts.clear();
    m_currentScript.clear();
    lua_close(m_L);
    m_L = nullptr;
}

bool LuaManager::LoadScriptMemory(const string& path, char* buffer, usize size)
{
    if (!m_L || !buffer || size == 0)
    {
        LOGGER_LOG_ERROR("[Lua] Invalid state or empty buffer for '%s'", path.c_str());
        return false;
    }

    m_currentScript = path;

    int base = lua_gettop(m_L);
    int status = luaL_loadbufferx(m_L, buffer, size, path.c_str(), nullptr);
    if (status != LUA_OK)
    {
        LuaError::Print(m_L, path, "LOAD ERROR");
        lua_settop(m_L, base);
        m_currentScript.clear();
        return false;
    }

    lua_pushcfunction(m_L, LuaError::Traceback);
    lua_insert(m_L, -2);
    int errFunc = base + 1;

    auto startTime = std::chrono::steady_clock::now();
    int64 memStart = LuaHeapBytes(m_L);

    status = lua_pcall(m_L, 0, 0, errFunc);

    if (m_profiler.enabled)
    {
        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - startTime).count();
        m_profiler.Record(0, path, ms, LuaHeapBytes(m_L) - memStart);
    }

    if (status != LUA_OK)
    {
        LuaError::Print(m_L, path, "ERROR");
        lua_settop(m_L, base);
        m_currentScript.clear();
        return false;
    }

    m_scripts[path] = true;
    lua_settop(m_L, base);
    m_currentScript.clear();
    return true;
}

bool LuaManager::LoadScript(const string& path)
{
    if (!m_L)
        return false;

    File file;
    if (!file.Open(path, FILE_MODE_READ))
    {
        LOGGER_LOG_ERROR("[Lua] Failed to open script: %s", path.c_str());
        return false;
    }

    uint32 size = file.GetSize();
    if (size == 0)
    {
        LOGGER_LOG_ERROR("[Lua] File empty: %s", path.c_str());
        return false;
    }

    std::vector<char> buffer(size);
    if (file.Read(buffer.data(), size) != size)
    {
        LOGGER_LOG_ERROR("[Lua] Read error: %s", path.c_str());
        return false;
    }

    return LoadScriptMemory(path, buffer.data(), size);
}

uint32 LuaManager::LoadScriptsFromDirectory(const string& dirPath, bool recursive, const string& extension)
{
    if (!m_L)
        return 0;

    m_rootDir = dirPath;
    while (!m_rootDir.empty() && (m_rootDir.back() == '/' || m_rootDir.back() == '\\'))
    {
        m_rootDir.pop_back();
    }

    std::vector<string> files = ScanDirectoryFiles(dirPath, recursive, extension);
    uint32 loaded = 0;

    for (auto& filePath : files)
    {
        if (LoadScript(filePath))
        {
            loaded++;
        }
    }

    LOGGER_LOG_INFO("[Lua] Loaded %u/%u scripts from '%s'", loaded, files.size(), dirPath.c_str());
    return loaded;
}

bool LuaManager::UnloadScript(const string& path)
{
    if (!m_L)
        return false;

    m_callbackRegistry.UnregisterScript(m_L, path);
    return m_scripts.erase(path) > 0;
}

bool LuaManager::ReloadScript(const string& path)
{
    if (!m_L)
        return false;

    bool wasLoaded = IsLoaded(path);
    uint32 before = m_callbackRegistry.CountForScript(path);

    uint32 idBoundary = m_callbackRegistry.PeekNextID();
    auto start = std::chrono::steady_clock::now();

    bool ok = LoadScript(path);

    if (ok)
    {
        m_callbackRegistry.UnregisterScript(m_L, path, 0, idBoundary);
    }
    else
    {
        m_callbackRegistry.UnregisterScript(m_L, path, idBoundary, 0xFFFFFFFF);
    }

    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    uint32 after = m_callbackRegistry.CountForScript(path);

    if (ok)
    {
        LOGGER_LOG_INFO("[Lua] %s %s (%.2f ms) callbacks %u -> %u", wasLoaded ? "RELOADED" : "LOADED", path.c_str(), ms,
                        before, after);
    }
    else
    {
        LOGGER_LOG_ERROR("[Lua] FAILED %s (%.2f ms) %s", path.c_str(), ms,
                         wasLoaded ? "previous version kept" : "script was not loaded");
    }

    return ok;
}

string LuaManager::FindLoadedScript(const string& query)
{
    string lowerQuery = ToLower(query);
    if (lowerQuery.empty())
        return "";

    string queryExt = GetFileExtension(lowerQuery).empty() ? lowerQuery + ".lua" : lowerQuery;
    string rooted = m_rootDir.empty() ? "" : ToLower(m_rootDir) + "/" + queryExt;

    for (auto& pair : m_scripts)
    {
        string lowerPath = ToLower(pair.first);
        if (lowerPath == lowerQuery || lowerPath == queryExt || (!rooted.empty() && lowerPath == rooted))
            return pair.first;
    }
    return "";
}

string LuaManager::FindOnDisk(const string& query)
{
    if (query.empty())
        return "";

    std::vector<string> names{query};
    if (GetFileExtension(query).empty())
    {
        names.push_back(query + ".lua");
    }

    std::vector<string> tries;
    if (!m_rootDir.empty())
    {
        for (auto& name : names)
        {
            tries.push_back(m_rootDir + "/" + name);
        }
    }
    for (auto& name : names)
    {
        tries.push_back(name);
    }

    for (auto& tryPath : tries)
    {
        File file;
        if (file.Open(tryPath, FILE_MODE_READ))
            return tryPath;
    }
    return "";
}

bool LuaManager::ReloadByName(const string& name)
{
    if (!m_L)
        return false;

    string path = FindLoadedScript(name);
    if (!path.empty())
        return ReloadScript(path);

    string onDisk = FindOnDisk(name);
    if (onDisk.empty())
    {
        LOGGER_LOG_ERROR("[Lua] No loaded script matches '%s' and no such file exists on disk", name.c_str());
        return false;
    }
    return ReloadScript(onDisk);
}

uint32 LuaManager::Reload(const string& target)
{
    if (target.empty() || target == "*")
        return ReloadAll();

    return ReloadByName(target) ? 1 : 0;
}

uint32 LuaManager::ReloadAll()
{
    if (!m_L)
        return 0;

    std::vector<string> scripts;
    for (auto& pair : m_scripts)
    {
        scripts.push_back(pair.first);
    }
    std::sort(scripts.begin(), scripts.end());

    LOGGER_LOG_INFO("[Lua] Reloading %u scripts...", scripts.size());

    uint32 ok = 0;
    for (auto& path : scripts)
    {
        if (ReloadScript(path))
        {
            ok++;
        }
    }

    LOGGER_LOG_INFO("[Lua] Reload finished: %u ok, %u failed", ok, scripts.size() - ok);
    return ok;
}

string LuaManager::DumpProfile(LuaProfileSort sort, usize topN)
{
    return m_profiler.Dump(sort, topN);
}

void LuaManager::ResetProfile()
{
    m_profiler.Reset();
    LOGGER_LOG_INFO("[LuaProfiler] Statistics reset.");
}

void LuaManager::RegisterFunction(const char* luaName, lua_CFunction func)
{
    if (!m_L)
        return;

    lua_register(m_L, luaName, func);
}

void LuaManager::RegisterCallbackBinding(const char* globalLuaFuncName, uint32 eventHash, const char* eventName)
{
    if (!m_L || !globalLuaFuncName || globalLuaFuncName[0] == '\0')
        return;

    m_profiler.SetEventName(eventHash, eventName);

    lua_pushlightuserdata(m_L, this);
    lua_pushinteger(m_L, eventHash);

    lua_pushcclosure(
        m_L,
        [](lua_State* state) -> int
        {
            auto* manager = (LuaManager*)lua_touserdata(state, lua_upvalueindex(1));
            if (!manager)
                return 0;

            uint32 hash = lua_tointeger(state, lua_upvalueindex(2));
            if (!lua_isfunction(state, 1))
                return luaL_error(state, "Expected function as callback for event %u", hash);

            manager->GetCallbackRegistry().Register(state, hash, 1, manager->GetCurrentLoadingScript());
            return 0;
        },
        2);

    lua_setglobal(m_L, globalLuaFuncName);
}

uint32 LuaManager::RegisterCallback(uint32 eventHash, const string& explicitScriptPath)
{
    if (!m_L || !lua_isfunction(m_L, -1))
        return 0;

    string path = explicitScriptPath.empty() ? m_currentScript : explicitScriptPath;
    return m_callbackRegistry.Register(m_L, eventHash, lua_gettop(m_L), path);
}

bool LuaManager::UnregisterCallback(uint32 id)
{
    return m_L ? m_callbackRegistry.Unregister(m_L, id) : false;
}

LuaManager* GetLuaManager()
{
    return LuaManager::GetInstance();
}

#endif