#include "WorldRendererManager.h"
#include "Context.h"
#include "IO/Log.h"
#include "MasterBroadway.h"
#include "Utils/ResourceManager.h"
#include <memory>

WorldRendererManager::WorldRendererManager() : m_worldRenderer(&m_renderer) {}

WorldRendererManager::~WorldRendererManager()
{
    RenderJob job;
    while (m_readyToDrawQueue.try_dequeue(job))
    {
        SAFE_DELETE(job.pWorld);
    }
}

bool WorldRendererManager::Expired(Timer& timer, uint64 limitMs)
{
    return limitMs > 0 && timer.GetElapsedTime() >= limitMs;
}

bool WorldRendererManager::HasPendingWork() const
{
    return m_renderQueue.size_approx() > 0 || m_readyToDrawQueue.size_approx() > 0;
}

bool WorldRendererManager::AddTask(uint32 userID, uint32 worldID)
{
    if (m_renderQueue.size_approx() >= m_config.maxQueueSize)
    {
        LOGGER_LOG_ERROR("Render queue is full (%d items), rejecting world %d for user %d", m_renderQueue.size_approx(),
                         worldID, userID);
        GetMasterBroadway()->SendWorldRenderResult(false, userID, worldID);
        return false;
    }

    WorldRenderInfo renderInfo;
    renderInfo.reqTime = Time::GetSystemTime();
    renderInfo.userID = userID;
    renderInfo.worldID = worldID;

    m_renderQueue.enqueue(renderInfo);
    m_idleTimer.Reset();
    return true;
}

void WorldRendererManager::PushReadyJob(const RenderJob& job)
{
    m_readyToDrawQueue.enqueue(job);
}

void WorldRendererManager::Update()
{
    if (!ProcessReadyJob() && !ProcessRequest())
    {
        ReleaseIdleMemory();
    }
}

bool WorldRendererManager::ProcessReadyJob()
{
    if (!Expired(m_lastRenderTimer, m_config.minRenderIntervalMs))
        return false;

    RenderJob job;
    if (!m_readyToDrawQueue.try_dequeue(job))
        return false;

    m_lastRenderTimer.Reset();

    bool success = false;
    if (job.pWorld)
    {
        if (!m_resourcesLoaded)
        {
            m_resourcesLoaded = true;
            m_resourceAgeTimer.Reset();
        }
        m_renderMemoryHeld = true;

        success = m_worldRenderer.Draw(job.pWorld);
        LOGGER_LOG_INFO("World %d took %dms to draw", job.worldID, (int32)job.renderStartTimer.GetElapsedTime());
        SAFE_DELETE(job.pWorld);
    }

    GetMasterBroadway()->SendWorldRenderResult(success, job.userID, job.worldID);
    m_idleTimer.Reset();
    return true;
}

bool WorldRendererManager::ProcessRequest()
{
    if (m_readyToDrawQueue.size_approx() >= m_config.maxPendingJobs)
        return false;

    WorldRenderInfo renderInfo;
    if (!m_renderQueue.try_dequeue(renderInfo))
        return false;

    if (renderInfo.worldID == 0)
    {
        GetMasterBroadway()->SendWorldRenderResult(false, renderInfo.userID, renderInfo.worldID);
        return true;
    }

    if (Time::GetSystemTime() - renderInfo.reqTime > m_config.requestTimeoutMs)
    {
        LOGGER_LOG_ERROR("Render request timed out for world %d, skipping", renderInfo.worldID);
        GetMasterBroadway()->SendWorldRenderResult(false, renderInfo.userID, renderInfo.worldID);
        return true;
    }

    std::unique_ptr<WorldInfo> pWorld(new WorldInfo());
    if (!m_worldRenderer.LoadWorld(renderInfo.worldID, pWorld.get()))
    {
        LOGGER_LOG_ERROR("Failed to load world %d file, skipping", renderInfo.worldID);
        GetMasterBroadway()->SendWorldRenderResult(false, renderInfo.userID, renderInfo.worldID);
        return true;
    }

    RenderJob job;
    job.userID = renderInfo.userID;
    job.worldID = renderInfo.worldID;
    job.pWorld = pWorld.release();
    job.renderStartTimer.Reset();

    PushReadyJob(job);
    return true;
}

void WorldRendererManager::ReleaseIdleMemory()
{
    if ((!m_renderMemoryHeld && !m_resourcesLoaded) || HasPendingWork())
        return;

    if (m_resourcesLoaded &&
        (Expired(m_idleTimer, m_config.resourceIdleMs) || Expired(m_resourceAgeTimer, m_config.resourceMaxAgeMs)))
    {
        ReleaseAllResources();
        return;
    }

    if (m_renderMemoryHeld && Expired(m_idleTimer, m_config.renderMemoryIdleMs))
    {
        ReleaseRenderMemory();
    }
}

void WorldRendererManager::ReleaseRenderMemory()
{
    m_worldRenderer.ReleaseMemory();
    m_renderMemoryHeld = false;
    LOGGER_LOG_INFO("Renderer memory released");
}

void WorldRendererManager::ReleaseAllResources()
{
    m_worldRenderer.ReleaseMemory();
    GetResourceManager()->Kill();

    m_renderMemoryHeld = false;
    m_resourcesLoaded = false;
    m_idleTimer.Reset();
    m_resourceAgeTimer.Reset();
    LOGGER_LOG_INFO("Renderer memory and resource manager cache released");
}

WorldRendererManager* GetWorldRendererManager()
{
    return WorldRendererManager::GetInstance();
}