#include "BanDBTable.h"

void DatabaseBanExec(DatabasePool* pPool, QueryRequest& req, bool preapred, bool bulk)
{
    if (req.queryID < 0)
    {
        DatabaseExec(pPool, "", req, QUERY_FLAG_RETURN_RESULT); // fail query
        return;
    }

    TableQuery& query = sBanQueryTable[req.queryID];

    if (preapred)
    {
        query.flags |= QUERY_FLAG_PREPARED;
    }

    if (bulk)
    {
        query.flags |= QUERY_FLAG_BULK;
    }

    DatabaseExec(pPool, query.query, req, query.flags);
}

void DatabaseBanRemoveExec(DatabasePool* pPool, int32 modType, string mac, string gid, string sid, string vid,
                           string ip, QueryRequest& req)
{
    string query = "DELETE FROM Bans WHERE (";

    if (mac == "02:00:00:00:00:00")
        mac.clear();

    if (gid == "00000000000000000000000000000000")
        gid.clear();

    if (sid == "00000000000000000000000000000000")
        gid.clear();

    if (vid == "00000000000000000000000000000000")
        vid.clear();

    query += "IP = ?";
    req.AddData(ip);

    // check md5 that can cause error ig
    if (!sid.empty())
    {
        query += " OR SID = UNHEX(MD5(?))";
        req.AddData(sid);
    }

    if (!gid.empty())
    {
        query += " OR GID = UNHEX(MD5(?))";
        req.AddData(gid);
    }

    // mac is hardcoded to regedit lol
    if (!vid.empty())
    {
        query += " OR VID = UNHEX(MD5(?)))";
        req.AddData(vid);
    }
    else
    {
        query += " OR Mac = ?)";
        req.AddData(mac);
    }

    if (modType != 0)
    {
        query += " AND ModType = ?;";
        req.AddData(modType);
    }

    DatabaseExec(pPool, query, req, QUERY_FLAG_NONE);
}

void DatabaseBanAddExec(DatabasePool* pPool, uint32 userID, uint32 adminUserID, int32 punishType, int32 modType,
                        uint32 durationMin, const string& reason, const string& ip, int32 hash, int32 platformType,
                        const string& mac, const string& gid, const string& sid, const string& vid, QueryRequest& req)
{
    DatabaseBanRemoveExec(pPool, modType, mac, gid, sid, vid, ip, QueryRequest{0});

    string query =
        "INSERT INTO Bans SET UserID = ?, BannerID = ?, PunishType = ?, ModType = ?, BannedTime = NOW(), "
        "ExpireTime = ADDDATE(NOW(), INTERVAL ? MINUTE), Reason = ?, IP = ?, PlatformType = ?, Hash = ?, Mac = ?";

    req.AddData(userID, adminUserID, punishType, modType, durationMin);

    if (reason.size() > 254)
        req.AddData(reason.substr(254));
    else
        req.AddData(reason);

    req.AddData(ip, platformType, hash, mac);

    // check md5
    if (!gid.empty())
    {
        query += ", GID = UNHEX(MD5(?))";
        req.AddData(gid);
    }

    if (!sid.empty())
    {
        query += ", SID = UNHEX(MD5(?))";
        req.AddData(sid);
    }

    if (!vid.empty())
    {
        query += ", VID = UNHEX(MD5(?))";
        req.AddData(vid);
    }
    query += ";";

    DatabaseExec(pPool, query, req, QUERY_FLAG_NONE);
}
