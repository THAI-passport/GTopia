#pragma once

#include "../DatabasePool.h"

// clang-format off
static TableQuery sStatsQueryTable[] = {
    {"SELECT * FROM Stats;", QUERY_FLAG_RETURN_RESULT},
    {"INSERT INTO Stats (StatKey, Category, IntVal, FloatVal, StringVal) VALUES (?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE Category = VALUES(Category), IntVal = VALUES(IntVal), FloatVal = VALUES(FloatVal), StringVal = VALUES(StringVal);", QUERY_FLAG_NONE}
};
// clang-format on

enum eStatsDBQuery
{
    DB_STATS_LOAD_ALL,
    DB_STATS_SAVE_UPSERT
};

namespace StatsDB
{
QueryRequest LoadAll(uint32 ownerID = 0);
QueryRequest BulkSaveUpsert();
} // namespace StatsDB

void DatabaseStatsExec(DatabasePool* pPool, QueryRequest& req, bool prepared = false, bool bulk = false);