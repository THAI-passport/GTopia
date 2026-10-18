#pragma once

#include "../Precompiled.h"
#include "../Utils/Timer.h"
#include "../Utils/Variant.h"
#include <functional>

class TCPPacketReader;
class TCPPacketWriter;
class DatabasePool;
class QueryTaskResult;

enum eServerStatCategory
{
    SERVER_STAT_CAT_GLOBAL,
    SERVER_STAT_CAT_DAILY
};

struct ServerStatItem
{
    string key;
    eServerStatCategory category = SERVER_STAT_CAT_GLOBAL;
    Variant totalVal;
    Variant deltaVal;

    bool isTcpDirty = false;
    bool isDbDirty = false;
};

class StatsManager
{
public:
    typedef std::function<void(TCPPacketWriter&)> SendTCPPacketCallback;

public:
    StatsManager();
    ~StatsManager();

public:
    static StatsManager* GetInstance()
    {
        static StatsManager instance;
        return &instance;
    }

public:
    void SetSendTCPPacketCallback(SendTCPPacketCallback cb) { m_sendTCPCb = cb; }

    void AddInt(const string& key, int32 amount, eServerStatCategory category = SERVER_STAT_CAT_GLOBAL);
    void AddFloat(const string& key, float amount, eServerStatCategory category = SERVER_STAT_CAT_GLOBAL);
    void SetString(const string& key, const string& val, eServerStatCategory category = SERVER_STAT_CAT_GLOBAL);

    int32 GetINT(const string& key) const;
    float GetFloat(const string& key) const;
    string GetString(const string& key) const;

    static void LoadFromDatabaseCB(QueryTaskResult&& result);
    void LoadFromDatabase(DatabasePool* pDbPool);
    void FlushToDatabase(DatabasePool* pDbPool, bool skipChecks);

    bool IsLoaded() const { return m_isLoaded; }

    void OnTCPPacket(TCPPacketReader& reader);
    void SyncStatsViaTCP();
    void Update(
#ifdef SERVER_MASTER
        DatabasePool* pDbPool
#endif
    );

private:
    ServerStatItem& GetStat(const string& key, eServerStatCategory category);
    void SetTotalDirect(const string& key, const Variant& val, eServerStatCategory category = SERVER_STAT_CAT_GLOBAL);

private:
    std::unordered_map<string, ServerStatItem> m_stats;
    SendTCPPacketCallback m_sendTCPCb;

    Timer m_flushTimer;
    bool m_isLoaded;
#ifdef SERVER_MASTER
    Timer m_dbFlushTimer;
#endif
};

StatsManager* GetStatsManager();