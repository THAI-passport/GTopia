#pragma once

#include "../../Holiday/Holiday.h"
#include "../DatabasePool.h"

// clang-format off
static TableQuery sHolidaysQueryTable[] = {
    {"SELECT * FROM Holidays;", QUERY_FLAG_RETURN_RESULT},
    {"UPDATE Holidays SET Type = ?, ScheduleType = ?, ScheduleParam = ?, DurationDays = ?, StartTimeUnix = ?, IsActive = ? WHERE HolidayKey = ?;", QUERY_FLAG_NONE},
    {"INSERT INTO Holidays (HolidayKey, Type, ScheduleType, ScheduleParam, DurationDays, StartTimeUnix, IsActive) VALUES (?, ?, ?, ?, ?, ?, ?);", QUERY_FLAG_NONE},
    {"DELETE FROM Holidays WHERE HolidayKey = ?;", QUERY_FLAG_NONE}
};

static TableQuery sHolidayRulesQueryTable[] = {
    {"SELECT * FROM HolidayRules;", QUERY_FLAG_RETURN_RESULT},
    {"INSERT INTO HolidayRules (HolidayKey, RuleType, StatKey, OperatorType, Threshold) VALUES (?, ?, ?, ?, ?);", QUERY_FLAG_NONE},
    {"DELETE FROM HolidayRules WHERE HolidayKey = ? AND StatKey = ? AND RuleType = ?;", QUERY_FLAG_NONE},
    {"DELETE FROM HolidayRules WHERE HolidayKey = ?;", QUERY_FLAG_NONE}
};
// clang-format on

enum eHolidaysDBQuery
{
    DB_HOLIDAYS_LOAD_ALL,
    DB_HOLIDAYS_UPDATE,
    DB_HOLIDAYS_CREATE,
    DB_HOLIDAYS_DELETE
};

enum eHolidayRulesDBQuery
{
    DB_HOLIDAY_RULES_LOAD_ALL,
    DB_HOLIDAY_RULES_CREATE,
    DB_HOLIDAY_RULES_DELETE_RULE,
    DB_HOLIDAY_RULES_DELETE_ALL_BY_HOLIDAY
};

namespace HolidayDB
{
QueryRequest LoadAll(uint32 ownerID = 0);
QueryRequest Update(const HolidayData& data, uint32 ownerID = 0);
QueryRequest Create(const HolidayData& data, uint32 ownerID = 0);
QueryRequest Delete(const string& holidaykey, uint32 ownerID = 0);
} // namespace HolidayDB

namespace HolidayRulesDB
{
QueryRequest LoadAll(uint32 ownerID = 0);
QueryRequest Create(const HolidayRule& rule);
QueryRequest DeleteRule(const string& holidayKey, const string& statKey, int8 ruleType);
QueryRequest DeleteAllByHoliday(const string& holidayKey);
} // namespace HolidayRulesDB

void DatabaseHolidayExec(DatabasePool* pPool, QueryRequest& req, bool prepared = false, bool bulk = false);
void DatabaseHolidayRulesExec(DatabasePool* pPool, QueryRequest& req, bool prepared = false, bool bulk = false);