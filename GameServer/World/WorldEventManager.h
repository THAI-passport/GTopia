#pragma once

#include "Precompiled.h"
#include "Utils/Timer.h"

// boring asf

class World;

enum eWorldEventType
{
    WORLD_EVENT_CRYSTAL
};

struct WorldEventInfo
{
    eWorldEventType type;
    string name;
    string desc;
    string winMsg;
    string loseMsg;
    string audioStart;
    string audioLose;
    string audioMusic;

    int32 durationSec = 0;
    string holidayKey;
};

class WorldEvent
{
public:
    WorldEvent();
    ~WorldEvent();

public:
    bool Start(eWorldEventType type, World* pWorld);
    void Update();
    void End();

private:
    eWorldEventType m_eventType;
    World* m_pWorld;
    Timer m_eventTimer;

    std::vector<int32> m_trackedObjectIds;
};

class WorldEventManager
{
public:
    WorldEventManager();
    ~WorldEventManager();

public:
    WorldEventInfo* GetEventInfo(int32 eventType);

private:
    std::unordered_map<int32, WorldEventInfo> m_eventInfos;
    std::vector<WorldEvent> m_activeEvents;
};