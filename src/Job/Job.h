#pragma once

#include <string>
#include <semaphore>
#include <memory>
#include <vector>

#include "../Database/Database.h"
#include "../Minecraft/Minecraft.h"

inline std::counting_semaphore<65535> g_ConcurrentPings(65535);

namespace Job
{
    class CJob
    {
    public:
        CJob() : m_bValid(false) {}
        CJob(std::string szIp, uint16_t iPort = 25565)
            : m_szIp(std::move(szIp)), m_iPort(iPort), m_bValid(true) {}

        void Work();
        void Callback(std::shared_ptr<Minecraft::CMinecraftServer> Mc);

        std::string GetIp() const { return m_szIp; }
        uint16_t GetPort() const { return m_iPort; }
        bool IsValid() const { return m_bValid; }

    private:
        bool m_bValid = false;
        std::string m_szIp;
        uint16_t m_iPort = 25565;
    };

    void AddToQue(CJob Job);
    CJob GetJob();
    int GetJobCount();
    bool IsEmpty();
}