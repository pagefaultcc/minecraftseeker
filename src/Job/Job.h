#pragma once
#include <string>
#include <semaphore>

#include "../Database/Database.h"
#include "../Minecraft/Minecraft.h"

inline std::counting_semaphore<256> g_ConcurrentPings(100);

namespace Job
{
	class CJob
	{
	public:
		CJob() : m_bValid(false) {}
		CJob(std::string szIp, uint16_t iPort = 25565) : m_szIp(szIp), m_iPort(iPort), m_bValid(true) {}

		void Work()
		{
			Database::CRecord Record(m_szIp);

			g_ConcurrentPings.acquire();
			Minecraft::CMinecraftServer Server(m_szIp);
			g_ConcurrentPings.release();
			
			if (!Server.m_bOnline)
				return;
			
			Record.AddRecord(Server.m_vecPlayers);
			Database::GetDatabase()->PushRecord(&Record);
		}

		std::string GetIp() { return m_szIp; }
		bool IsValid() { return m_bValid; }

	private:
		bool m_bValid;
		std::string m_szIp;
		uint16_t m_iPort;
	};

	void AddToQue(CJob Job);

	CJob GetJob();
	int GetJobCount();
}