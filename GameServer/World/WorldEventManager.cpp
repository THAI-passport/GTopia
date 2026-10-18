#include "WorldEventManager.h"
#include "WorldManager.h"

WorldEvent::WorldEvent() {}

WorldEvent::~WorldEvent() {}

bool WorldEvent::Start(eWorldEventType type, World* pWorld)
{
    if (!pWorld)
        return false;

    WorldEventInfo* pEventInfo = GetWorldManager()->GetSpecialEventManager().GetEventInfo(type);
    if (!pEventInfo)
        return false;

    m_eventType = type;
    m_pWorld = pWorld;
    m_eventTimer.Set(pEventInfo->durationSec * 1000);

    switch (type)
    {
        case WORLD_EVENT_CRYSTAL:
        {
            break;
        }
    }

    return true;
}

void WorldEvent::Update()
{
    if (!m_pWorld)
        return;
}

void WorldEvent::End() {}

WorldEventManager::WorldEventManager()
{
    WorldEventInfo crystalInfo;
    crystalInfo.type = WORLD_EVENT_CRYSTAL;
    crystalInfo.name = "Beautiful Crystal";
    crystalInfo.durationSec = 30;
    m_eventInfos.insert_or_assign(crystalInfo.type, std::move(crystalInfo));
}

WorldEventManager::~WorldEventManager() {}

WorldEventInfo* WorldEventManager::GetEventInfo(int32 eventType)
{
    auto it = m_eventInfos.find(eventType);
    if (it != m_eventInfos.end())
        return &it->second;
    return nullptr;
}
