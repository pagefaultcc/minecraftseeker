#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdint>

#include <pqxx/pqxx>

#include "../Logging/Logging.h"

namespace Database
{
	struct SHit
	{
		std::string szIp;
		uint16_t iPort;
		std::string szUsername;
		long long iTimestamp;
	};

	class CRecord
	{
	public:
		CRecord(const std::string& szIP, const std::uint16_t& iPort = 25565) : m_szIP(szIP), m_iPort(iPort) {}

		void AddRecord(const std::string& szRecord) { m_szRecords.push_back(szRecord); }
		void AddRecord(const std::vector<std::string>& szRecords) { m_szRecords.insert(m_szRecords.end(), szRecords.begin(), szRecords.end()); }

		std::string GetIp() { return m_szIP; }
		std::uint16_t GetPort() { return m_iPort; }
		std::vector<std::string>* GetRecords() { return &m_szRecords; }

	private:
		std::string m_szIP;
		std::uint16_t m_iPort;
		std::vector<std::string> m_szRecords;
	};

	class CDatabase
	{
	public:
		CDatabase(const std::string& szDbUri);
		~CDatabase();

		bool IsEnabled() const;
		pqxx::connection* GetConnection();

		void PushRecord(CRecord* pRecord);

		void StartFlushThread();
		void StopFlushThread();

	private:
		void EnsureConnection();
		void FlushLoop();
		bool FlushBatch(const std::vector<SHit>& batch);

		std::string m_szDbUri;
		bool m_bEnabled = false;
		pqxx::connection* m_pConnection = nullptr;
		std::mutex m_ConnectionMutex;

		std::vector<SHit> m_Queue;
		std::mutex m_QueueMutex;
		std::condition_variable m_QueueCv;
		std::thread m_FlushThread;
		std::atomic<bool> m_bRunning{ false };

		static constexpr size_t MAX_BATCH_SIZE = 2000;
		static constexpr std::chrono::milliseconds FLUSH_INTERVAL{ 500 };
	};

	void Initialize(const std::string& szDbUri);
	CDatabase* GetDatabase();
}