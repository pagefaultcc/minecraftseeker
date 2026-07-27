#include "Database.h"

#include "../Logging/Logging.h"

#include <unordered_map>

static Database::CDatabase* g_pDatabase = nullptr;

static std::string EscapeJsonString(const std::string& szInput)
{
	std::string szOut;
	szOut.reserve(szInput.size());

	for (char c : szInput)
	{
		switch (c)
		{
			case '\"': szOut += "\\\""; break;
			case '\\': szOut += "\\\\"; break;
			case '\n': szOut += "\\n"; break;
			case '\r': szOut += "\\r"; break;
			case '\t': szOut += "\\t"; break;
			default:
				if (static_cast<unsigned char>(c) < 0x20)
					szOut += ' ';
				else
					szOut += c;
		}
	}

	return szOut;
}

Database::CDatabase::CDatabase() : m_pConnection(nullptr)
{
	EnsureConnection();
}

Database::CDatabase::~CDatabase()
{
	StopFlushThread();
	delete m_pConnection;
}

void Database::CDatabase::EnsureConnection()
{
	std::lock_guard<std::mutex> lock(m_ConnectionMutex);

	while (true)
	{
		try
		{
			if (!m_pConnection || !m_pConnection->is_open())
			{
				delete m_pConnection;
				m_pConnection = nullptr;
				m_pConnection = new pqxx::connection(DB_CONNECTION_URI);
			}
			break;
		}
		catch (const std::exception&)
		{
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
	}
}

pqxx::connection* Database::CDatabase::GetConnection()
{
	EnsureConnection();
	return m_pConnection;
}

void Database::CDatabase::PushRecord(CRecord* pRecord)
{
	if (pRecord->GetRecords()->empty())
		return;

	long long iNow = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();

	std::vector<SHit> hits;
	hits.reserve(pRecord->GetRecords()->size());

	for (auto& szName : *pRecord->GetRecords())
		hits.push_back({ pRecord->GetIp(), pRecord->GetPort(), szName, iNow });

	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_Queue.insert(m_Queue.end(), std::make_move_iterator(hits.begin()), std::make_move_iterator(hits.end()));
	}

	m_QueueCv.notify_one();

	SPDLOG_INFO("Queued record for IP: {}, found {} players.", pRecord->GetIp(), pRecord->GetRecords()->size());
}

void Database::CDatabase::StartFlushThread()
{
	if (m_bRunning)
		return;

	m_bRunning = true;
	m_FlushThread = std::thread(&CDatabase::FlushLoop, this);
}

void Database::CDatabase::StopFlushThread()
{
	if (!m_bRunning)
		return;

	m_bRunning = false;
	m_QueueCv.notify_all();

	if (m_FlushThread.joinable())
		m_FlushThread.join();
}

void Database::CDatabase::FlushLoop()
{
	std::vector<SHit> batch;

	while (m_bRunning)
	{
		{
			std::unique_lock<std::mutex> lock(m_QueueMutex);
			m_QueueCv.wait_for(lock, FLUSH_INTERVAL, [this]() {
				return m_Queue.size() >= MAX_BATCH_SIZE || !m_bRunning;
			});

			if (m_Queue.empty())
				continue;

			batch.swap(m_Queue);
		}

		while (!FlushBatch(batch))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

		batch.clear();
	}

	std::vector<SHit> remaining;
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		remaining.swap(m_Queue);
	}

	if (!remaining.empty())
		FlushBatch(remaining);
}

bool Database::CDatabase::FlushBatch(const std::vector<SHit>& batch)
{
	if (batch.empty())
		return true;

	struct SGroup
	{
		uint16_t iPort;
		long long iLastSeen;
		std::string szEntries;
	};

	std::unordered_map<std::string, SGroup> groups;
	groups.reserve(batch.size());

	for (const auto& hit : batch)
	{
		auto& group = groups[hit.szIp];

		if (group.szEntries.empty())
		{
			group.iPort = hit.iPort;
			group.iLastSeen = hit.iTimestamp;
		}
		else
		{
			group.szEntries += ",";
			if (hit.iTimestamp > group.iLastSeen)
				group.iLastSeen = hit.iTimestamp;
			group.iPort = hit.iPort;
		}

		group.szEntries += "{\"username\":\"" + EscapeJsonString(hit.szUsername) +
			"\",\"timestamp\":" + std::to_string(hit.iTimestamp) + "}";
	}

	EnsureConnection();

	try
	{
		pqxx::work txn(*m_pConnection);

		std::string sql = "INSERT INTO servers (ip, seen, last_seen, port) VALUES ";
		pqxx::params params;
		bool bFirst = true;
		int iIndex = 1;

		for (auto& [szIp, group] : groups)
		{
			if (!bFirst)
				sql += ",";
			bFirst = false;

			sql += "($" + std::to_string(iIndex) + ",$" + std::to_string(iIndex + 1) +
				"::jsonb,to_timestamp($" + std::to_string(iIndex + 2) + "),$" +
				std::to_string(iIndex + 3) + ")";

			params.append(szIp);
			params.append("[" + group.szEntries + "]");
			params.append(group.iLastSeen);
			params.append(group.iPort);

			iIndex += 4;
		}

		sql += " ON CONFLICT (ip) DO UPDATE SET "
			   "seen = servers.seen || EXCLUDED.seen, "
			   "last_seen = EXCLUDED.last_seen, "
			   "port = EXCLUDED.port";

		txn.exec(sql, params);
		txn.commit();
		return true;
	}
	catch (const pqxx::broken_connection&)
	{
		if (m_pConnection)
		{
			try { m_pConnection->close(); }
			catch (...) {}
		}
		return false;
	}
	catch (const std::exception& e)
	{
		SPDLOG_INFO("{}", e.what());
		return false;
	}
}

void Database::Initialize()
{
	g_pDatabase = new CDatabase();
	g_pDatabase->StartFlushThread();

	SPDLOG_INFO("Connected to database, name: {}", g_pDatabase->GetConnection()->dbname());
}

Database::CDatabase* Database::GetDatabase()
{
	return g_pDatabase;
}