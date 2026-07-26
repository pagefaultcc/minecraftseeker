#include "Worker.h"

#include "../Job/Job.h"

std::vector<Worker::CThread> g_Threads;

void Worker::Worker(CThread* curThread)
{
    while (!curThread->ShouldBeStopped())
    {
        Job::CJob job = Job::GetJob();

        if (!job.IsValid())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        job.Work();
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
}