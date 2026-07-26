#include "Database.h"

#include "../Logging/Logging.h"

Database::CDatabase g_Database;
std::mutex g_DatabaseMutex;

void Database::CDatabase::PushRecord(CRecord* pRecord)
{
	if (pRecord->GetRecords()->empty())
		return;

	g_DatabaseMutex.lock();

	SPDLOG_INFO("Adding record for IP: {}, found {} players.", pRecord->GetIp(), pRecord->GetRecords()->size());
	
	for (auto& Record : *pRecord->GetRecords())
	{
		PushRecordInternal(pRecord->GetIp(), pRecord->GetPort(), Record, 
			std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::system_clock::now().time_since_epoch()
			).count());
	}
	
	g_DatabaseMutex.unlock();
}

void Database::Initialize()
{
	g_Database = CDatabase();

	SPDLOG_INFO("Connected to database, name: {}", g_Database.GetConnection()->dbname());
}

Database::CDatabase* Database::GetDatabase()
{
	return &g_Database;
}