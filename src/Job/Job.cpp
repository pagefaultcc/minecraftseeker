#include "Job.h"

#include "../ServerSeeker/ServerSeeker.h"

#include <deque>

std::deque<Job::CJob> g_Jobs;
std::mutex g_Mutex;

void Job::AddToQue(CJob Job)
{
    std::lock_guard<std::mutex> lock(g_Mutex);
	g_Jobs.push_back(Job);
}

Job::CJob Job::GetJob()
{
    std::lock_guard<std::mutex> lock(g_Mutex);

    if (g_Jobs.empty())
        return CJob();

    CJob job = std::move(g_Jobs.front());

    if (SS::GetConfig()->ShouldLoop())
        g_Jobs.push_back(g_Jobs.front());

    g_Jobs.pop_front();

    return job;
}

int Job::GetJobCount()
{
    return g_Jobs.size();
}