#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace Output
{
    struct SHitRecord
    {
        std::string szIp;
        uint16_t iPort;
        std::vector<std::string> vecPlayers;
        long long iTimestamp;
    };

    class COutput
    {
    public:
        COutput(const std::string& szFilePath);
        ~COutput();

        bool IsEnabled() const;
        void PushRecord(const std::string& szIp, uint16_t iPort, const std::vector<std::string>& vecPlayers, long long iTimestamp);

    private:
        std::string m_szFilePath;
        bool m_bJson = false;
        bool m_bEnabled = false;
        std::ofstream m_FileStream;
        std::mutex m_FileMutex;
    };

    void Initialize(const std::string& szFilePath);
    COutput* GetOutput();
}
