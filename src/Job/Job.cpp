#include "Job.h"
#include "../ServerSeeker/ServerSeeker.h"
#include "../Worker/Worker.h"
#include "../Output/Output.h"
#include "../Logging/Logging.h"

#include <deque>
#include <mutex>

static std::deque<Job::CJob> g_Jobs;
static std::mutex g_JobsMutex;

void Job::AddToQue(CJob Job)
{
    std::lock_guard<std::mutex> lock(g_JobsMutex);
    g_Jobs.push_back(Job);
}

Job::CJob Job::GetJob()
{
    std::lock_guard<std::mutex> lock(g_JobsMutex);

    if (g_Jobs.empty())
        return CJob();

    if (SS::GetConfig()->ShouldLoop())
        g_Jobs.push_back(g_Jobs.front());

    CJob job = std::move(g_Jobs.front());
    g_Jobs.pop_front();

    return job;
}

int Job::GetJobCount()
{
    std::lock_guard<std::mutex> lock(g_JobsMutex);
    return static_cast<int>(g_Jobs.size());
}

bool Job::IsEmpty()
{
    std::lock_guard<std::mutex> lock(g_JobsMutex);
    return g_Jobs.empty();
}

void Job::CJob::Work()
{
    g_ConcurrentPings.acquire();

    auto pServer = Minecraft::CMinecraftServer::Create(m_szIp, m_iPort);

    pServer->Ping(
        [this](std::shared_ptr<Minecraft::CMinecraftServer> mc)
        {
            Callback(mc);
        }
    );

    pServer->Run();
    g_ConcurrentPings.release();
}

void Job::CJob::Callback(std::shared_ptr<Minecraft::CMinecraftServer> Mc)
{
    if (!Mc || !Mc->m_bOnline)
        return;

    long long iNow = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    Worker::RecordHit(m_szIp, m_iPort, Mc->m_vecPlayers, iNow);

    if (!Mc->m_vecPlayers.empty())
    {
        Database::CRecord Record(m_szIp, m_iPort);
        Record.AddRecord(Mc->m_vecPlayers);
        if (Database::GetDatabase() && Database::GetDatabase()->IsEnabled())
            Database::GetDatabase()->PushRecord(&Record);

        if (Output::GetOutput() && Output::GetOutput()->IsEnabled())
            Output::GetOutput()->PushRecord(m_szIp, m_iPort, Mc->m_vecPlayers, iNow);

        SPDLOG_INFO("Hit: {}:{} | Found {} player(s)", m_szIp, m_iPort, Mc->m_vecPlayers.size());
    }
}