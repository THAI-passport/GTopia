#include "StatsManager.h"
#include "../Database/Table/StatsDBTable.h"
#include "../Packet/GamePacket.h"
#include "../Packet/TCPPacket.h"

StatsManager::StatsManager() : m_sendTCPCb(nullptr), m_isLoaded(false) {}

StatsManager::~StatsManager() {}

ServerStatItem& StatsManager::GetStat(const string& key, eServerStatCategory category)
{
    auto it = m_stats.find(key);
    if (it != m_stats.end())
        return it->second;

    auto res = m_stats.emplace(key, ServerStatItem{key, category, {}, {}, false});
    return res.first->second;
}

void StatsManager::AddInt(const string& key, int32 amount, eServerStatCategory category)
{
    auto& stat = GetStat(key, category);
    stat.isDbDirty = true;
    stat.isTcpDirty = true;

    stat.totalVal = stat.totalVal.GetINT() + amount;

#ifndef SERVER_MASTER
    stat.deltaVal = stat.deltaVal.GetINT() + amount;
#endif
}

void StatsManager::AddFloat(const string& key, float amount, eServerStatCategory category)
{
    auto& stat = GetStat(key, category);
    stat.isDbDirty = true;
    stat.isTcpDirty = true;

    stat.totalVal = stat.totalVal.GetFloat() + amount;

#ifndef SERVER_MASTER
    stat.deltaVal = stat.deltaVal.GetFloat() + amount;
#endif
}

void StatsManager::SetString(const string& key, const string& val, eServerStatCategory category)
{
    auto& stat = GetStat(key, category);
    stat.isDbDirty = true;
    stat.isTcpDirty = true;

    stat.totalVal = val;
    stat.deltaVal = val;
}

int32 StatsManager::GetINT(const string& key) const
{
    auto it = m_stats.find(key);
    if (it != m_stats.end())
        return it->second.totalVal.GetINT();

    return 0;
}

float StatsManager::GetFloat(const string& key) const
{
    auto it = m_stats.find(key);
    if (it != m_stats.end())
        return it->second.totalVal.GetFloat();

    return 0.0f;
}

string StatsManager::GetString(const string& key) const
{
    auto it = m_stats.find(key);
    if (it != m_stats.end())
        return it->second.totalVal.GetString();

    return "";
}

void StatsManager::LoadFromDatabaseCB(QueryTaskResult&& result)
{
    if (result.status != QUERY_STATUS_OK || !result.result)
    {
        LOGGER_LOG_ERROR("Failed to load server stats from database!");
        return;
    }

    DatabaseResult* pRes = result.result;
    uint32 rowCount = pRes->GetRowCount();

    for (uint32 i = 0; i < rowCount; ++i)
    {
        string statKey = pRes->GetField("StatKey", i).GetString();
        eServerStatCategory category = (eServerStatCategory)pRes->GetField("Category", i).GetINT();

        int32 intVal = pRes->GetField("IntVal", i).GetINT();
        float floatVal = pRes->GetField("FloatVal", i).GetFloat();
        string stringVal = pRes->GetField("StringVal", i).GetString();

        if (!stringVal.empty())
        {
            GetStatsManager()->SetTotalDirect(statKey, Variant(stringVal), category);
        }
        else if (floatVal != 0.0f)
        {
            GetStatsManager()->SetTotalDirect(statKey, Variant(floatVal), category);
        }
        else
        {
            GetStatsManager()->SetTotalDirect(statKey, Variant(intVal), category);
        }
    }

    GetStatsManager()->m_isLoaded = true;
    LOGGER_LOG_INFO("Successfully loaded %d server stats into memory.", rowCount);
    result.Destroy();
}

void StatsManager::LoadFromDatabase(DatabasePool* pDbPool)
{
    if (!pDbPool)
        return;

    LOGGER_LOG_INFO("Loading server stats from database...");

    QueryRequest req = StatsDB::LoadAll();
    req.callback = StatsManager::LoadFromDatabaseCB;

    DatabaseStatsExec(pDbPool, req, false, false);
}

void StatsManager::FlushToDatabase(DatabasePool* pDbPool, bool skipChecks)
{
    if (!pDbPool)
        return;

    QueryRequest bulkReq = StatsDB::BulkSaveUpsert();
    bool processedAny = false;

    for (auto& [_, stat] : m_stats)
    {
        if (!stat.isDbDirty && !skipChecks)
            continue;

        int32 intVal = 0;
        float floatVal = 0.0f;
        string stringVal = "";

        uint8 type = stat.totalVal.GetType();
        if (type == VARIANT_TYPE_INT)
        {
            intVal = stat.totalVal.GetINT();
        }
        else if (type == VARIANT_TYPE_FLOAT)
        {
            floatVal = stat.totalVal.GetFloat();
        }
        else if (type == VARIANT_TYPE_STRING)
        {
            stringVal = stat.totalVal.GetString();
        }

        bulkReq.AddData(stat.key, (int32)stat.category, intVal, floatVal, stringVal);
        stat.isDbDirty = false;
        processedAny = true;
    }

    if (processedAny || skipChecks)
    {
        DatabaseStatsExec(pDbPool, bulkReq, true, true);
    }
}

void StatsManager::OnTCPPacket(TCPPacketReader& reader)
{
    uint32 count = 0;
    if (!reader.Read(count) || count == 0)
        return;

    for (uint32 i = 0; i < count; ++i)
    {
        string key;
        int8 cat = 0;
        int8 type = 0;

        if (!reader.ReadString(key) || !reader.Read(cat) || !reader.Read(type))
            break;

        eServerStatCategory category = (eServerStatCategory)cat;

        if (type == VARIANT_TYPE_INT)
        {
            int32 delta = 0;
            reader.Read(delta);
#ifdef SERVER_MASTER
            AddInt(key, delta, category);
#else
            SetTotalDirect(key, delta, category);
#endif
        }
        else if (type == VARIANT_TYPE_FLOAT)
        {
            float delta = 0.0f;
            reader.Read(delta);
#ifdef SERVER_MASTER
            AddFloat(key, delta, category);
#else
            SetTotalDirect(key, delta, category);
#endif
        }
        else if (type == VARIANT_TYPE_STRING)
        {
            string str;
            reader.ReadString(str);
#ifdef SERVER_MASTER
            SetString(key, str, category);
#else
            SetTotalDirect(key, str, category);
#endif
        }
    }
}

void StatsManager::SyncStatsViaTCP()
{
    if (!m_sendTCPCb || m_stats.empty())
        return;

    TCPPacketWriter writer(TCP_PACKET_STATS_UPDATE);
    writer.Write((uint32)m_stats.size());

    for (auto& [_, stat] : m_stats)
    {
        writer.WriteString(stat.key);
        writer.Write<int8>(stat.category);

        int8 type = stat.totalVal.GetType();
        writer.Write<int8>(type);

        if (type == VARIANT_TYPE_INT)
        {
            writer.Write(stat.totalVal.GetINT());
        }
        else if (type == VARIANT_TYPE_FLOAT)
        {
            writer.Write(stat.totalVal.GetFloat());
        }
        else if (type == VARIANT_TYPE_STRING)
        {
            writer.WriteString(stat.totalVal.GetString());
        }
    }

    m_sendTCPCb(writer);
}

void StatsManager::Update(
#ifdef SERVER_MASTER
    DatabasePool* pDbPool
#endif
)
{
#ifdef SERVER_MASTER
    if (m_dbFlushTimer.GetElapsedTime() >= 60000)
    {
        FlushToDatabase(pDbPool, false);
        m_dbFlushTimer.Reset();
    }
#endif

    if (m_flushTimer.GetElapsedTime() < 10000)
        return;

#ifndef SERVER_MASTER
    if (!m_sendTCPCb)
    {
        m_flushTimer.Reset();
        return;
    }
#endif

    uint32 dirtyCount = 0;
    for (auto& [_, stat] : m_stats)
    {
        if (stat.isTcpDirty)
            dirtyCount++;
    }

    if (dirtyCount > 0)
    {
        TCPPacketWriter writer(TCP_PACKET_STATS_UPDATE);
        writer.Write(dirtyCount);

        for (auto& [_, stat] : m_stats)
        {
            if (!stat.isTcpDirty)
                continue;

            writer.WriteString(stat.key);
            writer.Write<int8>(stat.category);

            int8 type = stat.totalVal.GetType();
            writer.Write<int8>(type);

            if (type == VARIANT_TYPE_INT)
            {
#ifdef SERVER_MASTER
                writer.Write(stat.totalVal.GetINT());
#else
                writer.Write(stat.deltaVal.GetINT());
                stat.deltaVal = 0;
#endif
            }
            else if (type == VARIANT_TYPE_FLOAT)
            {
#ifdef SERVER_MASTER
                writer.Write(stat.totalVal.GetFloat());
#else
                writer.Write(stat.deltaVal.GetFloat());
                stat.deltaVal = 0.0f;
#endif
            }
            else if (type == VARIANT_TYPE_STRING)
            {
                writer.WriteString(stat.totalVal.GetString());
            }

            stat.isTcpDirty = false;
        }

        if (m_sendTCPCb)
        {
            m_sendTCPCb(writer);
        }

#ifdef SERVER_MASTER
        if (m_dbFlushTimer.GetElapsedTime() < 60000)
            return;
        FlushToDatabase(pDbPool, false);
#endif
    }

    m_flushTimer.Reset();
}

void StatsManager::SetTotalDirect(const string& key, const Variant& val, eServerStatCategory category)
{
    auto& stat = GetStat(key, category);
    stat.totalVal = val;
    stat.deltaVal = 0;
    stat.isTcpDirty = false;
    stat.isDbDirty = false;
}

StatsManager* GetStatsManager()
{
    return StatsManager::GetInstance();
}