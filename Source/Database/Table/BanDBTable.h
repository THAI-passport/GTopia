#pragma once

#include "../DatabasePool.h"

// clang-format off
static TableQuery sBanQueryTable[] =
{
    {"", QUERY_FLAG_RETURN_RESULT}
};
// clang-format on

enum eBanDBQuery
{
};

namespace BanDB
{

} // namespace BanDB

void DatabaseBanExec(DatabasePool* pPool, QueryRequest& req, bool preapred = false, bool bulk = false);
void DatabaseBanRemoveExec(DatabasePool* pPool, int32 modType, string mac, string gid, string sid, string vid,
                           string ip, QueryRequest& req);

void DatabaseBanAddExec(DatabasePool* pPool, uint32 userID, uint32 adminUserID, int32 punishType, int32 modType,
                        uint32 durationMin, const string& reason, const string& ip, int32 hash, int32 platformType,
                        const string& mac, const string& gid, const string& sid, const string& vid, QueryRequest& req);