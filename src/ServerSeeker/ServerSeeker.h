#pragma once

#include <string>
#include <cstdint>

namespace SS
{
    class Config
    {
    public:
        Config(int argc, char** argv);

        bool ShouldLoop() const { return m_bLoop; }
        int GetThreadCount() const { return m_iThreadCount; }
        uint16_t GetMinecraftPort() const { return m_iMinecraftPort; }
        uint16_t GetMonitoringPort() const { return m_iMonitoringPort; }
        std::string GetFilePath() const { return m_szFilePath; }
        std::string GetOutputFile() const { return m_szOutputFile; }
        std::string GetDatabaseUri() const { return m_szDbUri; }
        int GetTotalIpsLoaded() const { return m_iTotalIpsLoaded; }

    private:
        void PrintUsage(const std::string& szProgramName, const std::string& szErrorMsg = "");
        void LoadIps();

        bool m_bLoop = false;
        int m_iThreadCount = 256;
        uint16_t m_iMinecraftPort = 25565;
        uint16_t m_iMonitoringPort = 1337;
        std::string m_szFilePath;
        std::string m_szOutputFile;
        std::string m_szDbUri;
        int m_iTotalIpsLoaded = 0;
    };

    void Initialize(int argc, char** argv);
    Config* GetConfig();
}
