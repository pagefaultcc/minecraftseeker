#pragma once

// this is just a threader only for same function. not a good example but
//										it should be good because job uses workers.

#include <thread>
#include <functional>
#include "../Job/Job.h"

namespace Worker
{
	class CThread
	{
	public:
		CThread() : m_tThread(nullptr), m_bWorking(false), m_bRespectfullyStopReq(false) {}

		CThread(std::function<void(CThread*)> fnWork)
			: m_fnWork(fnWork), m_tThread(nullptr), m_bWorking(false), m_bRespectfullyStopReq(false)
		{
		}
	
		void Detach()
		{
			if (m_tThread) 
				return;

			m_tThread = new std::thread(m_fnWork, this);
			m_bWorking = true;
			m_tThread->detach();
		}

		bool operator!() const
		{
			return !m_tThread;
		}

		bool IsWorking() { return m_bWorking; }

		bool ShouldBeStopped() { return m_bRespectfullyStopReq; }
		void Stop() { m_bRespectfullyStopReq = true; }

		void SetCurrentJob(Job::CJob Job) { m_jCurrentJob = std::move(Job); }
		Job::CJob GetCurrentJob() { return m_jCurrentJob; }

	private:
		std::function<void(CThread*)>   m_fnWork;
		std::thread*                    m_tThread = nullptr;
		Job::CJob						m_jCurrentJob;

		bool                            m_bWorking = false;
		bool                            m_bRespectfullyStopReq = false; // Maybe in feature i add force kill option just in case.
	};

	void Worker(CThread* curThread);

	void Initialize(int iThreadCount);
	void Start();

	std::vector<Worker::CThread>* GetThreads();
}