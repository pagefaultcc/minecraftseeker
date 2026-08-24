#include "Worker.h"
#include "../Job/Job.h"
#include "../ServerSeeker/ServerSeeker.h"
#include "../Logging/Logging.h"

#include <algorithm>

static std::vector<Worker::CThread> g_Threads;
static Worker::SMetrics g_Metrics;
static std::thread g_StatusThread;
static std::atomic<bool> g_bRunning{ false };

static std::mutex g_RecentHitsMutex;
static std::vector<Worker::SHitInfo> g_RecentHits;

void Worker::RecordHit(const std::string& szIp, uint16_t iPort, const std::vector<std::string>& vecPlayers, long long iTimestamp)
{
	g_Metrics.m_iServersFound++;
	g_Metrics.m_iPlayersFound += vecPlayers.size();

	std::lock_guard<std::mutex> lock(g_RecentHitsMutex);
	g_RecentHits.push_back({ szIp, iPort, vecPlayers, iTimestamp });
	if (g_RecentHits.size() > 100)
		g_RecentHits.erase(g_RecentHits.begin());
}

std::vector<Worker::SHitInfo> Worker::GetRecentHits()
{
	std::lock_guard<std::mutex> lock(g_RecentHitsMutex);
	return g_RecentHits;
}

int Worker::GetActiveThreadCount()
{
	int iActive = 0;
	for (const auto& t : g_Threads)
		if (t.GetCurrentJob().IsValid())
			iActive++;
	return iActive;
}

void Worker::Worker(CThread* curThread)
{
	while (g_bRunning && !curThread->ShouldBeStopped())
	{
		Job::CJob job = Job::GetJob();
		curThread->SetCurrentJob(job);

		if (!job.IsValid())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			if (!SS::GetConfig()->ShouldLoop() && Job::IsEmpty() && Worker::GetActiveThreadCount() == 0)
				break;
			continue;
		}

		job.Work();
		g_Metrics.m_iServerPinged++;
	}
	curThread->SetCurrentJob(Job::CJob());
}

void Worker::StatusReporter()
{
	while (g_bRunning)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		if (!g_bRunning)
			break;

		long long iNow = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		long long iElapsed = std::max(1LL, iNow - g_Metrics.m_iStartTime);
		long long iPinged = g_Metrics.m_iServerPinged.load();
		long long iServersFound = g_Metrics.m_iServersFound.load();
		long long iPlayersFound = g_Metrics.m_iPlayersFound.load();
		double flRate = static_cast<double>(iPinged) / static_cast<double>(iElapsed);

		int iTotal = SS::GetConfig()->GetTotalIpsLoaded();
		if (SS::GetConfig()->ShouldLoop())
		{
			SPDLOG_INFO("Status: [Loop] | Rate: {:.1f} pings/s | Pinged: {} | Found: {} | Players: {} | Active: {}/{}",
				flRate, iPinged, iServersFound, iPlayersFound, GetActiveThreadCount(), SS::GetConfig()->GetThreadCount());
		}
		else
		{
			double flPercent = iTotal > 0 ? (static_cast<double>(iPinged) * 100.0 / static_cast<double>(iTotal)) : 0.0;
			if (flPercent > 100.0)
				flPercent = 100.0;

			long long iRemainingSec = 0;
			if (flRate > 0.0 && iPinged < iTotal)
				iRemainingSec = static_cast<long long>(static_cast<double>(iTotal - iPinged) / flRate);

			long long iHours = iRemainingSec / 3600;
			long long iMinutes = (iRemainingSec % 3600) / 60;
			long long iSeconds = iRemainingSec % 60;

			SPDLOG_INFO("Status: {:.2f}% | Rate: {:.1f} pings/s | Pinged: {}/{} | Found: {} | Players: {} | ETA: {:02d}:{:02d}:{:02d}",
				flPercent, flRate, iPinged, iTotal, iServersFound, iPlayersFound, iHours, iMinutes, iSeconds);

			if (iPinged >= iTotal && Job::IsEmpty() && GetActiveThreadCount() == 0)
			{
				SPDLOG_INFO("Scan finished. Total Pinged: {} | Found Servers: {} | Total Players: {} | Time: {}s",
					iPinged, iServersFound, iPlayersFound, iElapsed);
				break;
			}
		}
	}
}

void Worker::Initialize(int iThreadCount)
{
	g_Threads.reserve(iThreadCount);
	for (int i = 0; i < iThreadCount; i++)
		g_Threads.push_back(CThread(Worker::Worker));

	SPDLOG_INFO("{} Worker(s) have been initialized.", iThreadCount);
}

void Worker::Start()
{
	g_bRunning = true;
	g_Metrics.m_iStartTime = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	for (auto& thread : g_Threads)
		thread.Detach();

	g_StatusThread = std::thread(Worker::StatusReporter);
}

void Worker::Stop()
{
	g_bRunning = false;
	for (auto& thread : g_Threads)
		thread.Stop();

	if (g_StatusThread.joinable())
		g_StatusThread.join();
}

std::vector<Worker::CThread>* Worker::GetThreads()
{
	return &g_Threads;
}

Worker::SMetrics* Worker::GetMetrics()
{
	return &g_Metrics;
}