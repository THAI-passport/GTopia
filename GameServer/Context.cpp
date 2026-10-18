#include "Context.h"
#include "Crash/CrashReport.h"
#include "Server/GameServer.h"
#include "Server/MasterBroadway.h"

Context::Context() : m_pDbPool(nullptr) {}

Context::~Context() {}

void Context::Init()
{
    ContextBase::Init();

    CrashHandler::Initialize();
    InitializeCrashLogFile("GameServer", GetID());

    m_pDbPool = new DatabasePool();
    m_pGameConfig = new GameConfig();
}

void Context::Kill()
{
    ContextBase::Kill();
    CloseCrashLogFile();

    GetMasterBroadway()->Kill();
    GetGameServer()->Kill();

    SAFE_DELETE(m_pDbPool);
    SAFE_DELETE(m_pGameConfig);
}

void Context::SetupNetBurstFromConfig()
{
    if (!m_pGameConfig)
        return;

    gNetBurstConfig.threshold.heavyQueueSize = m_pGameConfig->netThreshold.heavyQueueSize;
    gNetBurstConfig.threshold.panicQueueSize = m_pGameConfig->netThreshold.panicQueueSize;
    gNetBurstConfig.threshold.heavyCpuPermille = m_pGameConfig->netThreshold.heavyCpuPermille;
    gNetBurstConfig.threshold.panicCpuPermille = m_pGameConfig->netThreshold.panicBurst;
    gNetBurstConfig.normalBurst = m_pGameConfig->netThreshold.normalBurst;
    gNetBurstConfig.heavyBurst = m_pGameConfig->netThreshold.heavyBurst;
    gNetBurstConfig.panicBurst = m_pGameConfig->netThreshold.panicBurst;
}

Context* GetContext()
{
    return Context::GetInstance();
}