#pragma once
#include <string>

#include "../Database/Database.h"
#include "../Minecraft/Minecraft.h"

namespace Job
{
	class CJob
	{
	public:
		CJob() : m_bValid(false) {}
		CJob(std::string szIp, uint16_t iPort = 25565) : m_szIp(szIp), m_iPort(iPort), m_bValid(true) {}

		void Work()
		{
			std::string cleanIp = m_szIp;
			cleanIp.erase(0, cleanIp.find_first_not_of(" \t\n\r\f\v"));
			cleanIp.erase(cleanIp.find_last_not_of(" \t\n\r\f\v") + 1);
			m_szIp = cleanIp;

			Database::CRecord Record(m_szIp);
			Minecraft::CMinecraftServer Server(m_szIp);
			
			if (!Server.m_bOnline)
				return;
			
			Record.AddRecord(Server.m_vecPlayers);
			Database::GetDatabase()->PushRecord(&Record);
		}

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