#pragma once

#include "Precompiled.h"
#include "Renderer2D.h"
#include "Utils/Timer.h"
#include "World/WorldInfo.h"
#include "WorldRenderer.h"
#include <concurrentqueue.h>

struct PlayerRenderData
{
    uint32 userID = 0;
    uint32 skinColor = 0;
    std::vector<int16> clothes;
};

struct WorldRenderInfo
{
    uint64 reqTime = 0;
    uint32 worldID = 0;
    uint32 userID = 0;
};

struct RenderJob
{
    uint32 userID = 0;
    uint32 worldID = 0;
    WorldInfo* pWorld = nullptr;
    Timer renderStartTimer;
    std::vector<PlayerRenderData> players;
};

struct WorldRendererManagerConfig
{
    usize maxQueueSize = 40;
    usize maxPendingJobs = 2;

    uint64 requestTimeoutMs = 5000;
    uint64 minRenderIntervalMs = 250;

    uint64 renderMemoryIdleMs = 10 * 60 * 1000;
    uint64 resourceIdleMs = 3 * 60 * 60 * 1000;
    uint64 resourceMaxAgeMs = 5 * 60 * 60 * 1000;
};

class WorldRendererManager
{
public:
    WorldRendererManager();
    ~WorldRendererManager();

public:
    static WorldRendererManager* GetInstance()
    {
        static WorldRendererManager instance;
        return &instance;
    }

public:
    void Update();
    bool AddTask(uint32 userID, uint32 worldID);
    void PushReadyJob(const RenderJob& job);

    void ReleaseRenderMemory();
    void ReleaseAllResources();

private:
    bool ProcessReadyJob();
    bool ProcessRequest();
    void ReleaseIdleMemory();
    bool HasPendingWork() const;

    static bool Expired(Timer& timer, uint64 limitMs);

private:
    WorldRendererManagerConfig m_config;

    moodycamel::ConcurrentQueue<WorldRenderInfo> m_renderQueue;
    moodycamel::ConcurrentQueue<RenderJob> m_readyToDrawQueue;

    Timer m_lastRenderTimer;
    Timer m_idleTimer;
    Timer m_resourceAgeTimer;

    bool m_renderMemoryHeld = false;
    bool m_resourcesLoaded = false;

    Renderer2D m_renderer;
    WorldRenderer m_worldRenderer;
};

WorldRendererManager* GetWorldRendererManager();