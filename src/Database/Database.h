#pragma once
#include <string>
#include <vector>

#include <pqxx/pqxx>

#include "../Logging/Logging.h"
#include "../../CONFIGURATION.h"

namespace Database
{
	class CRecord
	{
	public:
		CRecord(const std::string& szIP, const std::uint16_t& iPort = 25565) : m_szIP(szIP), m_iPort(iPort) {}

		void AddRecord(const std::string& szRecord) { m_szRecords.push_back(szRecord); }
		void AddRecord(const std::vector<std::string> szRecords) { m_szRecords.insert(m_szRecords.end(), szRecords.begin(), szRecords.end()); }
		
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
		CDatabase() : m_pConnection(nullptr)
		{
			EnsureConnection();
		}

		~CDatabase()
		{

		}

		pqxx::connection* GetConnection() 
		{ 
			EnsureConnection();
			return m_pConnection; 
		}

		void PushRecord(CRecord* pRecord);

	private:
		void EnsureConnection()
		{
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
				catch (const std::exception& e)
				{
					std::this_thread::sleep_for(std::chrono::seconds(2));
				}
			}
		}

		void PushRecordInternal(const std::string& szIp, const uint16_t& iPort, const std::string& szUsername, long long iTimeStamp)
		{
			EnsureConnection();

			try
			{
				pqxx::work txn(*m_pConnection);

				std::string new_entry =
					"{\"username\":\"" + txn.esc(szUsername) + "\",\"timestamp\":" + std::to_string(iTimeStamp) + "}";

				pqxx::result check = txn.exec("SELECT id FROM servers WHERE ip = $1", pqxx::params{ szIp });

				if (check.empty())
				{
					txn.exec(
						"INSERT INTO servers (ip, seen, last_seen, port) VALUES ($1, jsonb_build_array($2::jsonb), to_timestamp($3), $4)",
						pqxx::params{ szIp, new_entry, iTimeStamp, iPort }
					);
				}
				else
				{
					txn.exec(
						"UPDATE servers SET seen = seen || $1::jsonb, last_seen = to_timestamp($2) WHERE ip = $3",
						pqxx::params{ new_entry, iTimeStamp, szIp }
					);
				}

				txn.commit();
			}
			catch (const pqxx::broken_connection& e)
			{
				if (m_pConnection) 
					try { m_pConnection->close(); } catch (...) {}

				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				PushRecordInternal(szIp, iPort, szUsername, iTimeStamp);
			}
			catch (const std::exception& e)
			{
				SPDLOG_INFO("Database error: {}", e.what());
			}
		}

		pqxx::connection* m_pConnection;
	};

	void Initialize();
	CDatabase* GetDatabase();
}