#pragma once

#include "../Precompiled.h"

enum eHolidayType
{
    HOLIDAY_TYPE_MAIN = 0,
    HOLIDAY_TYPE_AUTO = 1
};

enum eHolidayScheduleType
{
    HOLIDAY_SCHED_NONE = 0,
    HOLIDAY_SCHED_UNIX_TIME = 1,
    HOLIDAY_SCHED_MONTHLY = 2,
    HOLIDAY_SCHED_YEARLY = 3,
    HOLIDAY_SCHED_RANDOM = 4
};

enum eRuleType
{
    RULE_TYPE_START = 0,
    RULE_TYPE_END = 1
};

enum eRuleOperator
{
    RULE_OP_GREATER_EQUAL = 0,
    RULE_OP_GREATER = 1,
    RULE_OP_LESS_EQUAL = 2,
    RULE_OP_LESS = 3,
    RULE_OP_EQUAL = 4
};

struct HolidayRule
{
    uint32 id = 0;
    string holidayKey;
    uint8 ruleType = RULE_TYPE_START;
    string statKey;
    eRuleOperator operatorType = RULE_OP_GREATER_EQUAL;
    int32 threshold = 0;

    bool CheckRule(int32 currentStatValue) const
    {
        switch (operatorType)
        {
            case RULE_OP_GREATER_EQUAL:
                return currentStatValue >= threshold;
            case RULE_OP_GREATER:
                return currentStatValue > threshold;
            case RULE_OP_LESS_EQUAL:
                return currentStatValue <= threshold;
            case RULE_OP_LESS:
                return currentStatValue < threshold;
            case RULE_OP_EQUAL:
                return currentStatValue == threshold;
            default:
                return false;
        }
    }
};

struct HolidayData
{
    uint32 id = 0;
    string holidayKey;
    uint32 durationDays = 1;
    eHolidayType type = HOLIDAY_TYPE_MAIN;
    uint32 startTimeUnix = 0;
    bool isActive = false;

    eHolidayScheduleType scheduleType = HOLIDAY_SCHED_NONE;
    string scheduleParam;
    uint32 randomAssignedStartDay = 0;
    int32 lastRandomMonth = -1;
    std::vector<HolidayRule> rules;

    int32 schedMonth = 0;
    int32 schedDay = 0;
    int32 lastTriggeredMonth = -1;
    int32 lastTriggeredYear = -1;
    uint32 lastTriggeredStart = 0;
};

class BaseHoliday
{
public:
    BaseHoliday(const HolidayData& data) : m_data(data) {}
    virtual ~BaseHoliday() {}

public:
    virtual void OnStarted() {}
    virtual void OnEnded() {}
    virtual void OnUpdate() {}

    virtual string GetWelcomeMessage() const { return ""; }

    bool IsActive() const { return m_data.isActive; }

    void SetActive(bool active)
    {
        if (m_data.isActive == active)
            return;

        m_data.isActive = active;
        if (m_data.isActive)
            OnStarted();
        else
            OnEnded();
    }

    HolidayData& GetData() { return m_data; }
    const string& GetKey() const { return m_data.holidayKey; }

protected:
    HolidayData m_data;
};