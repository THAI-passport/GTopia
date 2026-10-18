#pragma once

#include "../IO/Log.h"
#include "../Precompiled.h"
#include "../Utils/Timer.h"
#include "DatabasePool.h"
#include <functional>

class DatabasePool;

struct DBBootTask
{
    std::string name;
    std::function<void(DatabasePool*)> loadFunc;
    std::function<bool()> isLoadedFunc;

    bool isLoaded = false;
    Timer timer;
    uint64 durationMs = 0;
};

class DatabaseBootstrapper
{
public:
    DatabaseBootstrapper() {}
    ~DatabaseBootstrapper() {}

public:
    DatabaseBootstrapper& AddTask(string name, std::function<void(DatabasePool*)> loadFunc,
                                  std::function<bool()> isLoadedFunc)
    {
        DBBootTask task;
        task.name = std::move(name);
        task.loadFunc = std::move(loadFunc);
        task.isLoadedFunc = std::move(isLoadedFunc);
        task.isLoaded = false;

        m_tasks.push_back(std::move(task));
        return *this;
    }

    inline bool Execute(DatabasePool* pDbPool, std::function<void(uint64)> processDbResultsCb,
                        const volatile sig_atomic_t* shutdownFlag = nullptr, uint32 timeoutMs = 15000)
    {
        if (!pDbPool || m_tasks.empty())
            return true;

        uint32 totalTasks = m_tasks.size();
        LOGGER_LOG_INFO_FLUSH("--------------------------------------------------");
        LOGGER_LOG_INFO_FLUSH("[DB Boot] Starting DB boot (%d modules)...", totalTasks);
        LOGGER_LOG_INFO_FLUSH("--------------------------------------------------");

        for (auto& task : m_tasks)
        {
            task.timer.Reset();
            if (task.loadFunc)
            {
                task.loadFunc(pDbPool);
            }
        }

        uint32 completedCount = 0;
        Timer totalTimer;

        while (completedCount < totalTasks && totalTimer.GetElapsedTime() < timeoutMs)
        {
            if (shutdownFlag && (*shutdownFlag == 1))
            {
                LOGGER_LOG_WARN_FLUSH("[DB Boot] Shutdown signal detected! Interrupting DB boot.");
                return false;
            }

            pDbPool->GetWorker(0)->Update();
            if (processDbResultsCb)
            {
                processDbResultsCb(50);
            }

            for (auto& task : m_tasks)
            {
                if (!task.isLoaded && task.isLoadedFunc && task.isLoadedFunc())
                {
                    task.isLoaded = true;
                    task.durationMs = task.timer.GetElapsedTime();
                    completedCount++;

                    LOGGER_LOG_INFO_FLUSH("[DB Boot] [%d/%d] OK  -> %s (Took %u ms)", completedCount, totalTasks,
                                          task.name.c_str(), task.durationMs);
                }
            }

            SleepMS(5);
        }

        LOGGER_LOG_INFO_FLUSH("--------------------------------------------------");

        if (completedCount == totalTasks)
        {
            LOGGER_LOG_INFO_FLUSH("[DB Boot] SUCCESS: All %d modules loaded in %u ms!", totalTasks,
                                  totalTimer.GetElapsedTime());
            LOGGER_LOG_INFO_FLUSH("--------------------------------------------------");
            return true;
        }

        LOGGER_LOG_ERROR_FLUSH("[DB Boot] ERROR: Bootstrapping FAILED after %d ms! (%d/%d loaded)", timeoutMs,
                               completedCount, totalTasks);

        for (auto& task : m_tasks)
        {
            if (!task.isLoaded)
            {
                LOGGER_LOG_ERROR_FLUSH("[DB Boot] FAILED MODULE -> '%s' did not complete loading!", task.name.c_str());
            }
        }

        LOGGER_LOG_INFO_FLUSH("--------------------------------------------------");
        return false;
    }

private:
    std::vector<DBBootTask> m_tasks;
};