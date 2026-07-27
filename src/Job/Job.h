#pragma once
#include <string>
#include <semaphore>
#include <memory>

#include "../Database/Database.h"
#include "../Minecraft/Minecraft.h"

inline std::counting_semaphore<256> g_ConcurrentPings(500);

namespace Job
{
    class CJob
    {
    public:
        CJob() : m_bValid(false) {}
        CJob(std::string szIp, uint16_t iPort = 25565)
            : m_szIp(std::move(szIp)), m_iPort(iPort), m_bValid(true) {}

        void Work()
        {
            g_ConcurrentPings.acquire();

            auto server = Minecraft::CMinecraftServer::Create(m_szIp, m_iPort);

            server->Ping(
                [this](std::shared_ptr<Minecraft::CMinecraftServer> mc)
                {
                    Callback(mc);
                }
            );

            server->Run();
            g_ConcurrentPings.release();
        }

        void Callback(std::shared_ptr<Minecraft::CMinecraftServer> Mc)
        {
             if (!Mc.get()->m_bOnline)
                return;

            Database::CRecord Record(m_szIp);
            Record.AddRecord(Mc.get()->m_vecPlayers);
            Database::GetDatabase()->PushRecord(&Record);
        }

        std::string GetIp() const { return m_szIp; }
        bool IsValid() const { return m_bValid; }

    private:
        bool m_bValid = false;
        std::string m_szIp;
        uint16_t m_iPort = 25565;
    };

    void AddToQue(CJob Job);
    CJob GetJob();
    int GetJobCount();
}