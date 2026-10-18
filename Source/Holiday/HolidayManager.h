#pragma once

#include "../Precompiled.h"
#include "../Utils/Timer.h"
#include "Holiday.h"
#include <functional>

class DatabasePool;
class TCPPacketWriter;
class TCPPacketReader;
class QueryTaskResult;
class GameConfig;

class HolidayManager
{
public:
    typedef std::function<void(TCPPacketWriter&)> SendTCPPacketCallback;
    typedef std::function<BaseHoliday*(const HolidayData&)> HolidayCreator;

public:
    HolidayManager();
    ~HolidayManager();

public:
    static HolidayManager* GetInstance()
    {
        static HolidayManager instance;
        return &instance;
    }

public:
    void Update();

    void StartHoliday(const string& holidayKey, bool useScheduledStart = false);
    void EndHoliday(const string& holidayKey);

    bool IsHolidayActive(const string& holidayKey) const;
    BaseHoliday* GetHoliday(const string& holidayKey);
    string GetHolidayWelcomeMessages();

    BaseHoliday* CreateHolidayInstance(const HolidayData& data);
    void RegisterHoliday(BaseHoliday* pHoliday);

    void LoadFromDatabase(DatabasePool* pDbPool);
    static void LoadHolidaysCB(QueryTaskResult&& result);
    static void LoadRulesCB(QueryTaskResult&& result);

    void OnTCPPacket(TCPPacketReader& reader);
    void CheckScheduleAndRules();

    void AssignRandomScheduleForNextMonth(HolidayData& data, int32 currentYear, int32 currentMonth,
                                          bool allowCurrentMonth);

    void SetSendTCPPacketCallback(SendTCPPacketCallback cb) { m_sendTCPCb = cb; }
    void SetGameConfig(GameConfig* pConfig) { m_pGameConfig = pConfig; }
    void SetDatabasePool(DatabasePool* pDbPool) { m_pDbPool = pDbPool; }

    bool IsLoaded() const { return m_holidaysLoaded && m_rulesLoaded; }
    const std::unordered_map<string, BaseHoliday*>& GetHolidays() const { return m_holidays; }

#ifdef SERVER_MASTER
    void SyncHolidayViaTCP(const string& holidayKey);
    void SaveHolidayToDatabase(const string& holidayKey);

    void UpdateScheduleData(HolidayData& data, bool forceReassign = true);
    bool AdminCreateHoliday(HolidayData& data);
    bool AdminDeleteHoliday(const string& holidayKey);
    bool SetHolidayDuration(const string& holidayKey, uint32 durationDays);
    bool SetHolidayUnixTime(const string& holidayKey, uint32 startTimeUnix);
    bool SetHolidayType(const string& holidayKey, eHolidayType type);
    bool SetHolidaySchedule(const string& holidayKey, eHolidayScheduleType scheduleType,
                            const string& scheduleParam = "");
    bool AddHolidayRule(const string& holidayKey, const HolidayRule& rule);
    bool DeleteHolidayRule(const string& holidayKey, const string& statKey, uint8 ruleType);
    bool ClearHolidayRules(const string& holidayKey);
#endif

public:
    template <typename T> void RegisterCustomHoliday(const string& holidayKey)
    {
        m_creators[holidayKey] = [](const HolidayData& data) -> BaseHoliday* { return new T(data); };
    }

private:
    Timer m_checkTimer;
    GameConfig* m_pGameConfig;
    DatabasePool* m_pDbPool;
    SendTCPPacketCallback m_sendTCPCb;
    bool m_holidaysLoaded;
    bool m_rulesLoaded;

    std::unordered_map<string, BaseHoliday*> m_holidays;
    std::unordered_map<string, HolidayCreator> m_creators;
};

HolidayManager* GetHolidayManager();