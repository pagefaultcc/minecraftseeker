#include "Output.h"
#include "../Logging/Logging.h"
#include <algorithm>

static Output::COutput* g_pOutput = nullptr;

static bool EndsWithCaseInsensitive(const std::string& szStr, const std::string& szSuffix)
{
    if (szStr.length() < szSuffix.length())
        return false;

    return std::equal(szSuffix.rbegin(), szSuffix.rend(), szStr.rbegin(),
        [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); });
}

Output::COutput::COutput(const std::string& szFilePath)
    : m_szFilePath(szFilePath), m_bEnabled(false), m_bJson(false)
{
    if (m_szFilePath.empty())
        return;

    m_bJson = EndsWithCaseInsensitive(m_szFilePath, ".json");
    m_FileStream.open(m_szFilePath, std::ios::out | std::ios::app);

    if (!m_FileStream.is_open())
    {
        SPDLOG_ERROR("Failed to open output file: {}", m_szFilePath);
        return;
    }

    m_bEnabled = true;
    SPDLOG_INFO("Output file opened: {} (Format: {})", m_szFilePath, m_bJson ? "JSON" : "Text");
}

Output::COutput::~COutput()
{
    if (m_FileStream.is_open())
        m_FileStream.close();
}

bool Output::COutput::IsEnabled() const
{
    return m_bEnabled;
}

void Output::COutput::PushRecord(const std::string& szIp, uint16_t iPort, const std::vector<std::string>& vecPlayers, long long iTimestamp)
{
    if (!m_bEnabled)
        return;

    std::lock_guard<std::mutex> lock(m_FileMutex);

    if (m_bJson)
    {
        nlohmann::json obj;
        obj["ip"] = szIp;
        obj["port"] = iPort;
        obj["players"] = vecPlayers;
        obj["timestamp"] = iTimestamp;
        m_FileStream << obj.dump() << "\n";
        m_FileStream.flush();
    }
    else
    {
        std::string szLine = szIp + ":" + std::to_string(iPort) + " | Players (" + std::to_string(vecPlayers.size()) + "): ";
        for (size_t i = 0; i < vecPlayers.size(); ++i)
        {
            if (i > 0)
                szLine += ", ";
            szLine += vecPlayers[i];
        }
        szLine += " | Timestamp: " + std::to_string(iTimestamp) + "\n";
        m_FileStream << szLine;
        m_FileStream.flush();
    }
}

void Output::Initialize(const std::string& szFilePath)
{
    g_pOutput = new COutput(szFilePath);
}

Output::COutput* Output::GetOutput()
{
    return g_pOutput;
}
