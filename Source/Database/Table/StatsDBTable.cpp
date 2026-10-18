#include "StatsDBTable.h"

QueryRequest StatsDB::LoadAll(uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.queryID = DB_STATS_LOAD_ALL;
    return req;
}

QueryRequest StatsDB::BulkSaveUpsert()
{
    constexpr int32 PARAMS_PER_ROW = 5;

    QueryRequest req;
    req.queryID = DB_STATS_SAVE_UPSERT;

    req.AddData(PARAMS_PER_ROW);
    return req;
}

void DatabaseStatsExec(DatabasePool* pPool, QueryRequest& req, bool prepared, bool bulk)
{
    if (req.queryID < 0)
    {
        DatabaseExec(pPool, "", req, QUERY_FLAG_RETURN_RESULT);
        return;
    }

    TableQuery query = sStatsQueryTable[req.queryID];

    if (prepared)
    {
        query.flags |= QUERY_FLAG_PREPARED;
    }

    if (bulk)
    {
        query.flags |= QUERY_FLAG_BULK;
    }

    DatabaseExec(pPool, query.query, req, query.flags);
}