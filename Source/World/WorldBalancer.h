#pragma once

#include "../Precompiled.h"
#include "../Utils/GameConfig.h"

struct ActiveBalancedWorld
{
    string worldName;
    uint32 id;
    uint32 userCount;
};

class WorldBalancer
{
public:
    WorldBalancer();
    ~WorldBalancer();

public:
    virtual string WorldBalancerSelectBestActiveWorld(const string& worldName);

public:
    bool IsBalancerEnabled() const;
    void SetBalancerGameConfig(GameConfig* pConfig) { m_pGameConfig = pConfig; }

    void RegisterBalancedWorldsFromConfig();
    string BalancedWorldName(const string& inputName);
    WorldBalanceConfigSchema* GetBalancedWorldConfig(const string& inputName);
    uint32 GetBalancedWorldID(const string& inputName);
    uint32 WorldBalancerUpdateUserCount(const string& inputName, uint32 userCount);

private:
    void RegisterBalancedWorld(WorldBalanceConfigSchema& balancer);

private:
    bool m_enabled;
    GameConfig* m_pGameConfig;
    std::unordered_map<string, WorldBalanceConfigSchema> m_worlds;
    std::unordered_map<string, ActiveBalancedWorld> m_activeWorlds;
};