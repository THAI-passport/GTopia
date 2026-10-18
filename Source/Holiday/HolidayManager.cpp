#include "HolidayManager.h"
#include "../Database/Table/HolidayDBTable.h"
#include "../Math/Math.h"
#include "../Math/Random.h"
#include "../Packet/GamePacket.h"
#include "../Packet/TCPPacket.h"
#include "../Server/StatsManager.h"
#include "../Utils/GameConfig.h"
#include "../Utils/StringUtils.h"
#include <ctime>

static time_t MakeLocalMidnight(int32 year, int32 month, int32 day)
{
    tm timeInfo;
    timeInfo.tm_year = year - 1900;
    timeInfo.tm_mon = month - 1;
    timeInfo.tm_mday = day;
    timeInfo.tm_hour = 0;
    timeInfo.tm_min = 0;
    timeInfo.tm_sec = 0;
    timeInfo.tm_isdst = -1;
    return mktime(&timeInfo);
}

static uint32 GetEndTimeUnix(const HolidayData& data)
{
    time_t time = (time_t)data.startTimeUnix;
    tm timeInfo;
    GetTimeLocal(&timeInfo, &time);

    timeInfo.tm_mday += data.durationDays;
    timeInfo.tm_isdst = -1;

    return (uint32)mktime(&timeInfo);
}

static bool IsRuleTypeTriggered(const HolidayData& data, uint8 ruleType)
{
    for (auto& rule : data.rules)
    {
        if (rule.ruleType == ruleType && rule.CheckRule(GetStatsManager()->GetINT(rule.statKey)))
            return true;
    }
    return false;
}

HolidayManager::HolidayManager()
    : m_pGameConfig(nullptr), m_pDbPool(nullptr), m_holidaysLoaded(false), m_rulesLoaded(false)
{
}

HolidayManager::~HolidayManager()
{
    for (auto& [_, pHoliday] : m_holidays)
    {
        SAFE_DELETE(pHoliday);
    }
    m_holidays.clear();
}

void HolidayManager::Update()
{
#ifdef SERVER_MASTER
    if (m_checkTimer.GetElapsedTime() >= 5000)
    {
        CheckScheduleAndRules();
        m_checkTimer.Reset();
    }
#else
    for (auto& [_, pHoliday] : m_holidays)
    {
        if (pHoliday && pHoliday->IsActive())
        {
            pHoliday->OnUpdate();
        }
    }
#endif
}

void HolidayManager::StartHoliday(const string& holidayKey, bool useScheduledStart)
{
    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday || pHoliday->IsActive())
        return;

    HolidayData& data = pHoliday->GetData();

    if (!useScheduledStart || data.startTimeUnix == 0)
    {
        data.startTimeUnix = (uint32)Time::GetTimeSinceEpoch();
    }

    pHoliday->SetActive(true);

#ifdef SERVER_MASTER
    SaveHolidayToDatabase(data.holidayKey);
    SyncHolidayViaTCP(holidayKey);
#endif

    LOGGER_LOG_INFO("[HolidayManager] Started holiday: %s", holidayKey.c_str());
}

void HolidayManager::EndHoliday(const string& holidayKey)
{
    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday || !pHoliday->IsActive())
        return;

    HolidayData& data = pHoliday->GetData();
    pHoliday->SetActive(false);

#ifdef SERVER_MASTER
    SaveHolidayToDatabase(data.holidayKey);
    SyncHolidayViaTCP(holidayKey);
#endif

    LOGGER_LOG_INFO("[HolidayManager] Ended holiday: %s", holidayKey.c_str());
}

bool HolidayManager::IsHolidayActive(const string& holidayKey) const
{
    auto it = m_holidays.find(holidayKey);
    if (it != m_holidays.end() && it->second)
        return it->second->IsActive();

    return false;
}

BaseHoliday* HolidayManager::GetHoliday(const string& holidayKey)
{
    auto it = m_holidays.find(holidayKey);
    if (it != m_holidays.end())
        return it->second;

    return nullptr;
}

string HolidayManager::GetHolidayWelcomeMessages()
{
    string ret;

    for (auto& [_, pHoliday] : m_holidays)
    {
        if (!pHoliday || !pHoliday->IsActive())
            continue;

        string msg = pHoliday->GetWelcomeMessage();
        if (msg.empty())
            continue;

        if (!ret.empty())
            ret += "\n";

        ret += msg;
    }

    return ret;
}

BaseHoliday* HolidayManager::CreateHolidayInstance(const HolidayData& data)
{
    auto it = m_creators.find(data.holidayKey);
    if (it != m_creators.end())
        return it->second(data);

    return new BaseHoliday(data);
}

void HolidayManager::RegisterHoliday(BaseHoliday* pHoliday)
{
    if (!pHoliday)
        return;

    auto it = m_holidays.find(pHoliday->GetData().holidayKey);
    if (it != m_holidays.end())
    {
        if (it->second == pHoliday)
            return;

        SAFE_DELETE(it->second);
    }

    m_holidays[pHoliday->GetData().holidayKey] = pHoliday;
}

void HolidayManager::LoadHolidaysCB(QueryTaskResult&& result)
{
    if (result.status != QUERY_STATUS_OK || !result.result)
    {
        GetHolidayManager()->m_holidaysLoaded = true;
        return;
    }

    DatabaseResult* pRes = result.result;
    uint32 rowCount = pRes->GetRowCount();

    for (uint32 i = 0; i < rowCount; ++i)
    {
        HolidayData data;
        data.id = pRes->GetField("ID", i).GetINT();
        data.holidayKey = pRes->GetField("HolidayKey", i).GetString();
        data.durationDays = pRes->GetField("DurationDays", i).GetUINT();
        data.type = (eHolidayType)pRes->GetField("Type", i).GetINT();
        data.startTimeUnix = pRes->GetField("StartTimeUnix", i).GetUINT();
        data.isActive = (pRes->GetField("IsActive", i).GetINT() != 0);
        data.scheduleType = (eHolidayScheduleType)pRes->GetField("ScheduleType", i).GetINT();
        data.scheduleParam = pRes->GetField("ScheduleParam", i).GetString();

#ifdef SERVER_MASTER
        uint32 prevStart = data.startTimeUnix;
        string prevParam = data.scheduleParam;
        GetHolidayManager()->UpdateScheduleData(data, false);
        bool scheduleChanged = (prevStart != data.startTimeUnix) || (prevParam != data.scheduleParam);
#endif

        bool wasActive = data.isActive;
        data.isActive = false;

        BaseHoliday* pHoliday = GetHolidayManager()->CreateHolidayInstance(data);
        GetHolidayManager()->RegisterHoliday(pHoliday);

        if (wasActive)
        {
            pHoliday->SetActive(true);
        }

#ifdef SERVER_MASTER
        if (scheduleChanged)
        {
            GetHolidayManager()->SaveHolidayToDatabase(data.holidayKey);
        }
#endif
    }

    GetHolidayManager()->m_holidaysLoaded = true;
    result.Destroy();
}

void HolidayManager::LoadRulesCB(QueryTaskResult&& result)
{
    if (result.status != QUERY_STATUS_OK || !result.result)
    {
        GetHolidayManager()->m_rulesLoaded = true;
        return;
    }

    DatabaseResult* pRes = result.result;
    uint32 rowCount = pRes->GetRowCount();

    for (uint32 i = 0; i < rowCount; ++i)
    {
        HolidayRule rule;
        rule.id = pRes->GetField("ID", i).GetINT();
        rule.holidayKey = pRes->GetField("HolidayKey", i).GetString();
        rule.ruleType = (uint8)pRes->GetField("RuleType", i).GetINT();
        rule.statKey = pRes->GetField("StatKey", i).GetString();
        rule.operatorType = (eRuleOperator)pRes->GetField("OperatorType", i).GetINT();
        rule.threshold = pRes->GetField("Threshold", i).GetINT();

        BaseHoliday* pHoliday = GetHolidayManager()->GetHoliday(rule.holidayKey);
        if (pHoliday)
        {
            pHoliday->GetData().rules.push_back(rule);
        }
    }

    GetHolidayManager()->m_rulesLoaded = true;
    result.Destroy();
}

void HolidayManager::LoadFromDatabase(DatabasePool* pDbPool)
{
    if (!pDbPool)
        return;

    m_pDbPool = pDbPool;
    m_holidaysLoaded = false;
    m_rulesLoaded = false;

    QueryRequest holidayReq = HolidayDB::LoadAll();
    holidayReq.callback = HolidayManager::LoadHolidaysCB;
    DatabaseHolidayExec(pDbPool, holidayReq);

    QueryRequest ruleReq = HolidayRulesDB::LoadAll();
    ruleReq.callback = HolidayManager::LoadRulesCB;
    DatabaseHolidayRulesExec(pDbPool, ruleReq);
}

void HolidayManager::OnTCPPacket(TCPPacketReader& reader)
{
    uint32 count = 0;
    if (!reader.Read(count) || count == 0)
        return;

    for (uint32 i = 0; i < count; ++i)
    {
        string key;
        int8 type = 0;
        uint8 isActive = 0;
        uint32 startTimeUnix = 0;
        uint32 durationDays = 0;

        if (!reader.ReadString(key) || !reader.Read(type) || !reader.Read(isActive) || !reader.Read(startTimeUnix) ||
            !reader.Read(durationDays))
        {
            break;
        }

        BaseHoliday* pHoliday = GetHoliday(key);
        if (!pHoliday)
        {
            HolidayData data;
            data.holidayKey = key;
            data.type = (eHolidayType)type;
            data.durationDays = durationDays;
            data.isActive = false;
            data.startTimeUnix = startTimeUnix;

            pHoliday = CreateHolidayInstance(data);
            RegisterHoliday(pHoliday);
        }

        HolidayData& data = pHoliday->GetData();
        data.type = (eHolidayType)type;
        data.startTimeUnix = startTimeUnix;
        data.durationDays = durationDays;
        pHoliday->SetActive(isActive == 1);
    }
}

void HolidayManager::AssignRandomScheduleForNextMonth(HolidayData& data, int32 currentYear, int32 currentMonth,
                                                      bool allowCurrentMonth)
{
    time_t now = time(nullptr);
    int32 mon = 0, d = 0, y = 0, h = 0, min = 0, sec = 0;
    GetTimeLocal(&now, &mon, &d, &y, &h, &min, &sec);

    int32 duration = Max(1, (int32)data.durationDays);

    int32 targetMonth = currentMonth;
    int32 targetYear = currentYear;

    bool canUseCurrent = allowCurrentMonth && (currentMonth == mon) && (currentYear == y) &&
                         ((Time::GetDaysInMonth(y, mon) - d + 1) >= duration);

    if (!canUseCurrent)
    {
        targetMonth++;
        if (targetMonth > 12)
        {
            targetMonth = 1;
            targetYear++;
        }
    }

    int32 maxStartDay = Max(1, Time::GetDaysInMonth(targetYear, targetMonth) - duration + 1);
    int32 minStartDay = canUseCurrent ? d : 1;
    if (minStartDay > maxStartDay)
    {
        minStartDay = maxStartDay;
    }

    int32 randomStartDay = RandomRangeInt(minStartDay, maxStartDay);

    data.startTimeUnix = (uint32)MakeLocalMidnight(targetYear, targetMonth, randomStartDay);
    data.lastRandomMonth = targetMonth;
    data.randomAssignedStartDay = randomStartDay;
    data.lastTriggeredStart = 0;
}

void HolidayManager::CheckScheduleAndRules()
{
    if (!IsLoaded())
        return;

    time_t now = time(nullptr);
    int32 mon = 0, d = 0, y = 0, h = 0, min = 0, sec = 0;
    GetTimeLocal(&now, &mon, &d, &y, &h, &min, &sec);

    uint32 currentTimeUnix = (uint32)now;

    for (auto& [_, pHoliday] : m_holidays)
    {
        if (!pHoliday)
            continue;

        HolidayData& data = pHoliday->GetData();

        if (m_pGameConfig && !m_pGameConfig->IsHolidayEnabled(data.holidayKey))
        {
            if (data.isActive)
            {
                LOGGER_LOG_INFO("[HolidayManager] Holiday [%s] disabled in config.txt, ending...",
                                data.holidayKey.c_str());
                EndHoliday(data.holidayKey);
            }
            continue;
        }

        if (data.isActive)
        {
            if (currentTimeUnix < data.startTimeUnix || currentTimeUnix >= GetEndTimeUnix(data))
            {
                LOGGER_LOG_INFO("[HolidayManager] Holiday [%s] timing out of bounds, ending...",
                                data.holidayKey.c_str());
                EndHoliday(data.holidayKey);
                continue;
            }
        }

        if (!data.rules.empty())
        {
            if (!data.isActive)
            {
                if (IsRuleTypeTriggered(data, RULE_TYPE_START) && !IsRuleTypeTriggered(data, RULE_TYPE_END))
                {
                    StartHoliday(data.holidayKey);
                    continue;
                }
            }
            else
            {
                if (IsRuleTypeTriggered(data, RULE_TYPE_END))
                {
                    EndHoliday(data.holidayKey);
                    continue;
                }
            }
        }

        if (data.isActive)
            continue;

        switch (data.scheduleType)
        {
            case HOLIDAY_SCHED_UNIX_TIME:
            {
                if (data.startTimeUnix == 0)
                    break;

                if (currentTimeUnix >= data.startTimeUnix && currentTimeUnix < GetEndTimeUnix(data) &&
                    data.lastTriggeredStart != data.startTimeUnix)
                {
                    data.lastTriggeredStart = data.startTimeUnix;
                    StartHoliday(data.holidayKey, true);
                }
                break;
            }

            case HOLIDAY_SCHED_MONTHLY:
            {
                if (data.schedDay <= 0)
                    break;

                for (int32 offset = 0; offset >= -1; --offset)
                {
                    int32 m = mon + offset;
                    int32 yy = y;
                    if (m < 1)
                    {
                        m = 12;
                        yy--;
                    }

                    int32 day = Min(data.schedDay, Time::GetDaysInMonth(yy, m));

                    HolidayData probe = data;
                    probe.startTimeUnix = (uint32)MakeLocalMidnight(yy, m, day);
                    uint32 startUnix = probe.startTimeUnix;

                    if (currentTimeUnix >= startUnix && currentTimeUnix < GetEndTimeUnix(probe))
                    {
                        if (data.lastTriggeredStart != startUnix)
                        {
                            data.startTimeUnix = startUnix;
                            data.lastTriggeredStart = startUnix;
                            data.lastTriggeredMonth = m;
                            data.lastTriggeredYear = yy;
                            StartHoliday(data.holidayKey, true);
                        }
                        break;
                    }
                }
                break;
            }

            case HOLIDAY_SCHED_YEARLY:
            {
                if (data.schedMonth < 1 || data.schedMonth > 12 || data.schedDay <= 0)
                    break;

                for (int32 yy = y; yy >= y - 1; --yy)
                {
                    int32 day = Min(data.schedDay, Time::GetDaysInMonth(yy, data.schedMonth));

                    HolidayData probe = data;
                    probe.startTimeUnix = (uint32)MakeLocalMidnight(yy, data.schedMonth, day);
                    uint32 startUnix = probe.startTimeUnix;

                    if (currentTimeUnix >= startUnix && currentTimeUnix < GetEndTimeUnix(probe))
                    {
                        if (data.lastTriggeredStart != startUnix)
                        {
                            data.startTimeUnix = startUnix;
                            data.lastTriggeredStart = startUnix;
                            data.lastTriggeredYear = yy;
                            StartHoliday(data.holidayKey, true);
                        }
                        break;
                    }
                }
                break;
            }

            case HOLIDAY_SCHED_RANDOM:
            {
                if (data.startTimeUnix == 0 || currentTimeUnix >= GetEndTimeUnix(data))
                {
                    AssignRandomScheduleForNextMonth(data, y, mon, false);
#ifdef SERVER_MASTER
                    SaveHolidayToDatabase(data.holidayKey);
                    SyncHolidayViaTCP(data.holidayKey);
#endif
                }
                else if (currentTimeUnix >= data.startTimeUnix && data.lastTriggeredStart != data.startTimeUnix)
                {
                    data.lastTriggeredStart = data.startTimeUnix;
                    StartHoliday(data.holidayKey, true);
                }
                break;
            }

            default:
                break;
        }
    }
}

#ifdef SERVER_MASTER
void HolidayManager::SaveHolidayToDatabase(const string& holidayKey)
{
    if (!m_pDbPool)
        return;

    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday)
        return;

    QueryRequest req = HolidayDB::Update(pHoliday->GetData());
    DatabaseHolidayExec(m_pDbPool, req);
}

void HolidayManager::SyncHolidayViaTCP(const string& holidayKey)
{
    if (!m_sendTCPCb)
        return;

    TCPPacketWriter writer(TCP_PACKET_HOLIDAY_SYNC);

    if (holidayKey.empty())
    {
        uint32 validCount = 0;
        for (auto& [_, pHoliday] : m_holidays)
        {
            if (pHoliday)
                ++validCount;
        }

        writer.Write<uint32>(validCount);

        for (auto& [_, pHoliday] : m_holidays)
        {
            if (!pHoliday)
                continue;

            HolidayData& data = pHoliday->GetData();
            writer.WriteString(data.holidayKey);
            writer.Write<int8>((int8)data.type);
            writer.Write<uint8>(data.isActive ? 1 : 0);
            writer.Write(data.startTimeUnix);
            writer.Write(data.durationDays);
        }
    }
    else
    {
        BaseHoliday* pHoliday = GetHoliday(holidayKey);
        if (!pHoliday)
            return;

        writer.Write<uint32>(1);

        HolidayData& data = pHoliday->GetData();
        writer.WriteString(data.holidayKey);
        writer.Write<int8>((int8)data.type);
        writer.Write<uint8>(data.isActive ? 1 : 0);
        writer.Write(data.startTimeUnix);
        writer.Write(data.durationDays);
    }

    m_sendTCPCb(writer);
}

void HolidayManager::UpdateScheduleData(HolidayData& data, bool forceReassign)
{
    data.schedMonth = 0;
    data.schedDay = 0;

    if (data.scheduleType == HOLIDAY_SCHED_MONTHLY)
    {
        if (!data.scheduleParam.empty())
        {
            if (ToInt(data.scheduleParam, data.schedDay) != TO_INT_SUCCESS || data.schedDay <= 0 || data.schedDay > 31)
            {
                data.schedDay = -1;
                LOGGER_LOG_INFO("[HolidayManager] Failed to update schedule data for %s, failed to parse %s",
                                data.holidayKey.c_str(), data.scheduleParam.c_str());
            }
        }
    }
    else if (data.scheduleType == HOLIDAY_SCHED_YEARLY)
    {
        if (!data.scheduleParam.empty())
        {
            int32 m = 0, d = 0;
            if (sscanf(data.scheduleParam.c_str(), "%d-%d", &m, &d) == 2 && m >= 1 && m <= 12 && d >= 1 && d <= 31)
            {
                data.schedMonth = m;
                data.schedDay = d;
            }
            else
            {
                data.schedMonth = 0;
                data.schedDay = -1;
                LOGGER_LOG_INFO("[HolidayManager] Failed to update schedule data for %s, failed to parse %s",
                                data.holidayKey.c_str(), data.scheduleParam.c_str());
            }
        }
    }
    else if (data.scheduleType == HOLIDAY_SCHED_RANDOM)
    {
        time_t now = time(nullptr);

        if (!forceReassign && data.startTimeUnix > 0 && (uint32)now < GetEndTimeUnix(data))
            return;

        bool allowThisMonth = (data.scheduleParam == "current");
        data.scheduleParam.clear();

        int32 month = 0, day = 0, year = 0, hour = 0, minute = 0, second = 0;
        GetTimeLocal(&now, &month, &day, &year, &hour, &minute, &second);
        AssignRandomScheduleForNextMonth(data, year, month, allowThisMonth);
    }
}

bool HolidayManager::AdminCreateHoliday(HolidayData& data)
{
    if (GetHoliday(data.holidayKey))
        return false;

    UpdateScheduleData(data);

    BaseHoliday* pHoliday = CreateHolidayInstance(data);
    RegisterHoliday(pHoliday);

    if (m_pDbPool)
    {
        QueryRequest req = HolidayDB::Create(data);
        DatabaseHolidayExec(m_pDbPool, req);
    }

    SyncHolidayViaTCP(data.holidayKey);
    LOGGER_LOG_INFO("[HolidayManager] Created new holiday: %s", data.holidayKey.c_str());
    return true;
}

bool HolidayManager::AdminDeleteHoliday(const string& holidayKey)
{
    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday)
        return false;

    if (pHoliday->IsActive())
    {
        EndHoliday(holidayKey);
    }

    m_holidays.erase(holidayKey);
    SAFE_DELETE(pHoliday);

    if (m_pDbPool)
    {
        QueryRequest reqHoliday = HolidayDB::Delete(holidayKey);
        DatabaseHolidayExec(m_pDbPool, reqHoliday);

        QueryRequest reqRules = HolidayRulesDB::DeleteAllByHoliday(holidayKey);
        DatabaseHolidayRulesExec(m_pDbPool, reqRules);
    }

    LOGGER_LOG_INFO("[HolidayManager] Deleted holiday: %s", holidayKey.c_str());
    return true;
}

bool HolidayManager::SetHolidayDuration(const string& holidayKey, uint32 durationDays)
{
    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday)
        return false;

    pHoliday->GetData().durationDays = durationDays;
    SaveHolidayToDatabase(holidayKey);
    SyncHolidayViaTCP(holidayKey);

    CheckScheduleAndRules();
    return true;
}

bool HolidayManager::SetHolidayUnixTime(const string& holidayKey, uint32 startTimeUnix)
{
    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday)
        return false;

    HolidayData& data = pHoliday->GetData();
    data.startTimeUnix = startTimeUnix;
    data.lastTriggeredStart = 0;
    SaveHolidayToDatabase(holidayKey);
    SyncHolidayViaTCP(holidayKey);

    CheckScheduleAndRules();
    return true;
}

bool HolidayManager::SetHolidayType(const string& holidayKey, eHolidayType type)
{
    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday)
        return false;

    pHoliday->GetData().type = type;
    SaveHolidayToDatabase(holidayKey);
    SyncHolidayViaTCP(holidayKey);
    return true;
}

bool HolidayManager::SetHolidaySchedule(const string& holidayKey, eHolidayScheduleType scheduleType,
                                        const string& scheduleParam)
{
    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday)
        return false;

    HolidayData& data = pHoliday->GetData();
    data.scheduleType = scheduleType;
    data.scheduleParam = scheduleParam;
    data.lastTriggeredStart = 0;

    UpdateScheduleData(data);

    SaveHolidayToDatabase(holidayKey);
    SyncHolidayViaTCP(holidayKey);
    return true;
}

bool HolidayManager::AddHolidayRule(const string& holidayKey, const HolidayRule& rule)
{
    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday)
        return false;

    pHoliday->GetData().rules.push_back(rule);

    if (m_pDbPool)
    {
        QueryRequest req = HolidayRulesDB::Create(rule);
        DatabaseHolidayRulesExec(m_pDbPool, req);
    }
    return true;
}

bool HolidayManager::DeleteHolidayRule(const string& holidayKey, const string& statKey, uint8 ruleType)
{
    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday)
        return false;

    auto& rules = pHoliday->GetData().rules;
    auto it = std::remove_if(rules.begin(), rules.end(),
                             [&](const HolidayRule& r) { return r.statKey == statKey && r.ruleType == ruleType; });

    if (it != rules.end())
    {
        rules.erase(it, rules.end());
        if (m_pDbPool)
        {
            QueryRequest req = HolidayRulesDB::DeleteRule(holidayKey, statKey, ruleType);
            DatabaseHolidayRulesExec(m_pDbPool, req);
        }
        return true;
    }
    return false;
}

bool HolidayManager::ClearHolidayRules(const string& holidayKey)
{
    BaseHoliday* pHoliday = GetHoliday(holidayKey);
    if (!pHoliday)
        return false;

    pHoliday->GetData().rules.clear();

    if (m_pDbPool)
    {
        QueryRequest req = HolidayRulesDB::DeleteAllByHoliday(holidayKey);
        DatabaseHolidayRulesExec(m_pDbPool, req);
    }
    return true;
}
#endif

HolidayManager* GetHolidayManager()
{
    return HolidayManager::GetInstance();
}