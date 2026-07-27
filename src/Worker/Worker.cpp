#include "Worker.h"

#include "../Job/Job.h"

std::vector<Worker::CThread> g_Threads;

std::mutex g_MetricsMutex;
Worker::SMetrics g_Metrics; 

void Worker::Worker(CThread* curThread)
{
    while (!curThread->ShouldBeStopped())
    {
        Job::CJob job = Job::GetJob();
        curThread->SetCurrentJob(job);
        
        if (!job.IsValid())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        
        job.Work();

        std::lock_guard<std::mutex> lock(g_MetricsMutex);
        g_Metrics.m_iServerPinged++;
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
	for (auto& thread : g_Threads)
		thread.Detach();
    
    g_Metrics.m_iStartTime = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}

std::vector<Worker::CThread>* Worker::GetThreads()
{
    return &g_Threads;
}

Worker::SMetrics* Worker::GetMetrics()
{
    std::lock_guard<std::mutex> lock(g_MetricsMutex);
    return &g_Metrics;
}