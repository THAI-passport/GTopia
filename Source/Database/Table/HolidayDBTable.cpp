#include "HolidayDBTable.h"

QueryRequest HolidayDB::LoadAll(uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.queryID = DB_HOLIDAYS_LOAD_ALL;
    return req;
}

QueryRequest HolidayDB::Update(const HolidayData& data, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.queryID = DB_HOLIDAYS_UPDATE;

    req.AddData((int32)data.type, (int32)data.scheduleType, data.scheduleParam, data.durationDays, data.startTimeUnix,
                (int32)(data.isActive ? 1 : 0), data.holidayKey);
    return req;
}

QueryRequest HolidayDB::Create(const HolidayData& data, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.queryID = DB_HOLIDAYS_CREATE;

    req.AddData(data.holidayKey, (int32)data.type, (int32)data.scheduleType, data.scheduleParam, data.durationDays,
                data.startTimeUnix, (int32)(data.isActive ? 1 : 0));

    return req;
}

QueryRequest HolidayDB::Delete(const string& holidaykey, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.queryID = DB_HOLIDAYS_DELETE;

    req.AddData(holidaykey);
    return req;
}

QueryRequest HolidayRulesDB::LoadAll(uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.queryID = DB_HOLIDAY_RULES_LOAD_ALL;

    return req;
}

QueryRequest HolidayRulesDB::Create(const HolidayRule& rule)
{
    QueryRequest req;
    req.queryID = DB_HOLIDAY_RULES_CREATE;

    req.AddData(rule.holidayKey, (int32)rule.ruleType, rule.statKey, (int32)rule.operatorType, rule.threshold);
    return req;
}

QueryRequest HolidayRulesDB::DeleteRule(const string& holidayKey, const string& statKey, int8 ruleType)
{
    QueryRequest req;
    req.queryID = DB_HOLIDAY_RULES_DELETE_RULE;

    req.AddData(holidayKey, statKey, ruleType);
    return req;
}

QueryRequest HolidayRulesDB::DeleteAllByHoliday(const string& holidayKey)
{
    QueryRequest req;
    req.queryID = DB_HOLIDAY_RULES_DELETE_ALL_BY_HOLIDAY;

    req.AddData(holidayKey);
    return req;
}

void DatabaseHolidayExec(DatabasePool* pPool, QueryRequest& req, bool prepared, bool bulk)
{
    if (req.queryID < 0)
    {
        DatabaseExec(pPool, "", req, QUERY_FLAG_RETURN_RESULT);
        return;
    }

    TableQuery query = sHolidaysQueryTable[req.queryID];
    if (prepared)
        query.flags |= QUERY_FLAG_PREPARED;
    if (bulk)
        query.flags |= QUERY_FLAG_BULK;

    DatabaseExec(pPool, query.query, req, query.flags);
}

void DatabaseHolidayRulesExec(DatabasePool* pPool, QueryRequest& req, bool prepared, bool bulk)
{
    if (req.queryID < 0)
    {
        DatabaseExec(pPool, "", req, QUERY_FLAG_RETURN_RESULT);
        return;
    }

    TableQuery query = sHolidayRulesQueryTable[req.queryID];
    if (prepared)
        query.flags |= QUERY_FLAG_PREPARED;
    if (bulk)
        query.flags |= QUERY_FLAG_BULK;

    DatabaseExec(pPool, query.query, req, query.flags);
}