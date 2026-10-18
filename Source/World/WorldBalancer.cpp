#include "WorldBalancer.h"
#include "../Math/Random.h"
#include "../Utils/StringUtils.h"

WorldBalancer::WorldBalancer() : m_enabled(false) {}

WorldBalancer::~WorldBalancer() {}

string WorldBalancer::WorldBalancerSelectBestActiveWorld(const string& worldName)
{
    return "";
}

bool WorldBalancer::IsBalancerEnabled() const
{
    if (!m_pGameConfig)
        return false;

    return m_pGameConfig->isWorldBalancerEnabled;
}

void WorldBalancer::RegisterBalancedWorldsFromConfig()
{
    if (!m_pGameConfig)
        return;

    for (auto& balancer : m_pGameConfig->balancedWorlds)
    {
        RegisterBalancedWorld(balancer);
    }
}

void WorldBalancer::RegisterBalancedWorld(WorldBalanceConfigSchema& balancer)
{
    if (balancer.worldName.empty() || balancer.maxInstance == 0)
        return;

    balancer.id = (m_worlds.size() + 1);
    balancer.worldName = ToUpper(balancer.worldName);

    m_worlds.insert_or_assign(balancer.worldName, balancer);
}

WorldBalanceConfigSchema* WorldBalancer::GetBalancedWorldConfig(const string& inputName)
{
    if (inputName.empty() || m_worlds.empty())
        return nullptr;

    string worldName = ToUpper(inputName);

    auto isValidInstance = [](const string& suffix, uint32 maxInstance) -> bool
    {
        if (suffix.empty() || suffix[0] == '0')
            return false;

        uint32 instanceId = 0;
        if (ToUInt(suffix, instanceId) != TO_INT_SUCCESS)
            return false;

        return instanceId > 0 && instanceId <= maxInstance;
    };

    auto itExact = m_worlds.find(worldName);
    if (itExact != m_worlds.end())
        return itExact->second.keepExactId ? nullptr : &itExact->second;

    usize underscorePos = worldName.rfind('_');
    if (underscorePos != string::npos && underscorePos > 0)
    {
        auto it = m_worlds.find(worldName.substr(0, underscorePos));
        if (it != m_worlds.end() && it->second.keepExactId)
        {
            if (isValidInstance(worldName.substr(underscorePos + 1), it->second.maxInstance))
                return &it->second;
        }
        return nullptr;
    }

    usize digitStartPos = worldName.size();
    while (digitStartPos > 0 && IsDigit(worldName[digitStartPos - 1]))
        --digitStartPos;

    if (digitStartPos > 0 && digitStartPos < worldName.size())
    {
        auto it = m_worlds.find(worldName.substr(0, digitStartPos));
        if (it != m_worlds.end() && !it->second.keepExactId)
        {
            if (isValidInstance(worldName.substr(digitStartPos), it->second.maxInstance))
                return &it->second;
        }
    }

    return nullptr;
}

uint32 WorldBalancer::GetBalancedWorldID(const string& inputName)
{
    WorldBalanceConfigSchema* pConfig = GetBalancedWorldConfig(inputName);
    return pConfig ? pConfig->id : 0;
}

uint32 WorldBalancer::WorldBalancerUpdateUserCount(const string& inputName, uint32 userCount)
{
    if (inputName.empty())
        return 0;

    string worldName = ToUpper(inputName);

    auto itActive = m_activeWorlds.find(worldName);
    if (itActive != m_activeWorlds.end())
    {
        itActive->second.userCount = userCount;
        return userCount;
    }

    WorldBalanceConfigSchema* pConfig = GetBalancedWorldConfig(worldName);
    if (pConfig)
    {
        ActiveBalancedWorld newActiveWorld;
        newActiveWorld.worldName = worldName;
        newActiveWorld.id = pConfig->id;
        newActiveWorld.userCount = userCount;

        m_activeWorlds.emplace(worldName, std::move(newActiveWorld));
        return userCount;
    }

    return 0;
}

string WorldBalancer::BalancedWorldName(const string& inputName)
{
    if (inputName.empty())
        return "";

    string worldName = ToUpper(inputName);

    WorldBalanceConfigSchema* pConfig = GetBalancedWorldConfig(worldName);
    if (!pConfig)
        return worldName;

    string candidateName;
    candidateName.reserve(pConfig->worldName.size() + 16);

    uint32 minUsers = 99999;
    uint32 bestInstanceId = 1;

    for (uint32 i = 1; i <= pConfig->maxInstance; ++i)
    {
        candidateName = pConfig->worldName;
        if (pConfig->keepExactId)
            candidateName.push_back('_');

        candidateName += ToString(i);

        uint32 currentCount = 0;
        auto it = m_activeWorlds.find(candidateName);
        if (it != m_activeWorlds.end())
        {
            currentCount = it->second.userCount;
        }

        if (currentCount < minUsers)
        {
            minUsers = currentCount;
            bestInstanceId = i;

            if (minUsers == 0)
                break;
        }
    }

    string result = pConfig->worldName;
    if (pConfig->keepExactId)
        result.push_back('_');

    result += ToString(bestInstanceId);
    return result;
}