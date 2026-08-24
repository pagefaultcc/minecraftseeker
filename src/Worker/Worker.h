#pragma once

#include <chrono>
#include <thread>
#include <functional>
#include <vector>
#include <atomic>
#include <mutex>
#include <string>

#include "../Job/Job.h"

namespace Worker
{
	struct SHitInfo
	{
		std::string szIp;
		uint16_t iPort;
		std::vector<std::string> vecPlayers;
		long long iTimestamp;
	};

	struct SMetrics
	{
		std::atomic<long long> m_iServerPinged{ 0 };
		std::atomic<long long> m_iServersFound{ 0 };
		std::atomic<long long> m_iPlayersFound{ 0 };
		long long m_iStartTime{ 0 };
	};

	class CThread
	{
	public:
		CThread() : m_pThread(nullptr), m_bWorking(false), m_bRespectfullyStopReq(false) {}

		CThread(std::function<void(CThread*)> fnWork)
			: m_fnWork(fnWork), m_pThread(nullptr), m_bWorking(false), m_bRespectfullyStopReq(false)
		{
		}

		void Detach()
		{
			if (m_pThread)
				return;

			m_pThread = new std::thread(m_fnWork, this);
			m_bWorking = true;
			m_pThread->detach();
		}

		bool operator!() const
		{
			return !m_pThread;
		}

		bool IsWorking() const { return m_bWorking; }

		bool ShouldBeStopped() const { return m_bRespectfullyStopReq; }
		void Stop() { m_bRespectfullyStopReq = true; }

		void SetCurrentJob(Job::CJob Job) { m_jCurrentJob = std::move(Job); }
		Job::CJob GetCurrentJob() const { return m_jCurrentJob; }

	private:
		std::function<void(CThread*)> m_fnWork;
		std::thread* m_pThread = nullptr;
		Job::CJob m_jCurrentJob;
		bool m_bWorking = false;
		bool m_bRespectfullyStopReq = false;
	};

	void Worker(CThread* curThread);
	void StatusReporter();

	void Initialize(int iThreadCount);
	void Start();
	void Stop();

	void RecordHit(const std::string& szIp, uint16_t iPort, const std::vector<std::string>& vecPlayers, long long iTimestamp);

	std::vector<Worker::CThread>* GetThreads();
	SMetrics* GetMetrics();
	std::vector<SHitInfo> GetRecentHits();
	int GetActiveThreadCount();
}