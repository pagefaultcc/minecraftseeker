#include "ServerSeeker.h"
#include "../Logging/Logging.h"
#include "../Job/Job.h"

#include <args.hxx>
#include <algorithm>
#include <vector>

SS::Config* g_pConfig = nullptr;

void SS::Config::PrintUsage(const std::string& szProgramName, const std::string& szErrorMsg)
{
    if (!szErrorMsg.empty())
        SPDLOG_ERROR("{}", szErrorMsg);

    SPDLOG_INFO("Usage: {} [options] <ips.txt>", szProgramName);
    SPDLOG_INFO("Options:");
    SPDLOG_INFO("  -t,  --threads <num>            Number of worker threads (default: 256)");
    SPDLOG_INFO("  -f,  --file <path>              Path to file containing IP list");
    SPDLOG_INFO("  -p,  --port <port>              Minecraft server port (default: 25565)");
    SPDLOG_INFO("  -mp, --monitoring-port <port>   API monitoring port (default: 1337)");
    SPDLOG_INFO("  -o,  --output <path>            Output file path (.txt or .json)");
    SPDLOG_INFO("  -db, --database-uri <uri>       PostgreSQL connection URI");
    SPDLOG_INFO("  -l,  --loop                     Loop the IP list continuously");
    SPDLOG_INFO("  -h,  --help                     Display this help menu");
    SPDLOG_INFO("Examples:");
    SPDLOG_INFO("  {} -t 256 -f ips.txt", szProgramName);
    SPDLOG_INFO("  {} -t 512 -f ips.txt -p 25565 -o results.json", szProgramName);
    SPDLOG_INFO("  {} -t 256 -f ips.txt -o results.txt -mp 8080 -l", szProgramName);
    SPDLOG_INFO("  {} -t 512 -f ips.txt -db postgres://user:pass@localhost:5432/db", szProgramName);
    SPDLOG_INFO("  {} 256 ips.txt", szProgramName);
}

SS::Config::Config(int argc, char** argv)
{
    std::string szProgramName = argv[0];

    if (argc <= 1)
    {
        PrintUsage(szProgramName, "Wrong usage! Not enough arguments.");
        std::exit(-1);
    }

    std::vector<std::string> vecNormalizedArgs;
    for (int i = 0; i < argc; ++i)
    {
        std::string szArg = argv[i];
        if (szArg == "-mp")
            vecNormalizedArgs.push_back("--mp");
        else if (szArg.rfind("-mp=", 0) == 0)
            vecNormalizedArgs.push_back("--mp=" + szArg.substr(4));
        else if (szArg == "-db")
            vecNormalizedArgs.push_back("--db");
        else if (szArg.rfind("-db=", 0) == 0)
            vecNormalizedArgs.push_back("--db=" + szArg.substr(4));
        else
            vecNormalizedArgs.push_back(szArg);
    }

    std::vector<char*> vecArgv;
    for (auto& s : vecNormalizedArgs)
        vecArgv.push_back(s.data());

    args::ArgumentParser parser("ServerSeeker - Multi-threaded Minecraft server scanner.",
        "Examples:\n"
        "  serverseeker -t 256 -f ips.txt\n"
        "  serverseeker -t 512 -f ips.txt -p 25565 -o results.json\n"
        "  serverseeker -t 256 -f ips.txt -o results.txt -mp 8080 -l\n"
        "  serverseeker -t 512 -f ips.txt -db postgres://user:pass@localhost:5432/db\n"
        "  serverseeker 256 ips.txt");

    args::HelpFlag flagHelp(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<int> flagThreads(parser, "threads", "Number of worker threads (default: 256)", {'t', "threads"}, 256);
    args::ValueFlag<std::string> flagFile(parser, "file", "Path to file containing IP list", {'f', "file"});
    args::ValueFlag<uint16_t> flagPort(parser, "port", "Minecraft target port (default: 25565)", {'p', "port"}, 25565);
    args::ValueFlag<uint16_t> flagMonitoringPort(parser, "monitoring-port", "API monitoring port (default: 1337)", {"mp", "monitoring-port"}, 1337);
    args::ValueFlag<std::string> flagOutput(parser, "output", "Output file path (.txt or .json)", {'o', "output"});
    args::ValueFlag<std::string> flagDbUri(parser, "database-uri", "PostgreSQL connection URI", {"db", "database-uri"});
    args::Flag flagLoop(parser, "loop", "Loop the IP list continuously", {'l', "loop"});

    args::Positional<std::string> pos1(parser, "arg1", "Thread count or IP file path");
    args::Positional<std::string> pos2(parser, "arg2", "IP file path or loop flag");
    args::Positional<std::string> pos3(parser, "arg3", "Optional loop flag");

    try
    {
        parser.ParseCLI(static_cast<int>(vecArgv.size()), vecArgv.data());
    }
    catch (const args::Help&)
    {
        PrintUsage(szProgramName);
        std::exit(0);
    }
    catch (const args::ParseError& e)
    {
        PrintUsage(szProgramName, e.what());
        std::exit(-1);
    }
    catch (const args::ValidationError& e)
    {
        PrintUsage(szProgramName, e.what());
        std::exit(-1);
    }

    if (flagThreads)
        m_iThreadCount = args::get(flagThreads);

    if (flagFile)
        m_szFilePath = args::get(flagFile);

    if (flagPort)
        m_iMinecraftPort = args::get(flagPort);

    if (flagMonitoringPort)
        m_iMonitoringPort = args::get(flagMonitoringPort);

    if (flagOutput)
        m_szOutputFile = args::get(flagOutput);

    if (flagDbUri)
        m_szDbUri = args::get(flagDbUri);

    if (flagLoop)
        m_bLoop = true;

    if (pos1)
    {
        std::string szPos1 = args::get(pos1);
        bool bAllDigits = !szPos1.empty() && std::all_of(szPos1.begin(), szPos1.end(), ::isdigit);
        if (bAllDigits && !flagThreads)
            m_iThreadCount = std::stoi(szPos1);
        else if (m_szFilePath.empty())
            m_szFilePath = szPos1;
    }

    if (pos2)
    {
        std::string szPos2 = args::get(pos2);
        if (m_szFilePath.empty())
            m_szFilePath = szPos2;
        else if (szPos2 == "loop" || szPos2 == "true" || szPos2 == "1")
            m_bLoop = true;
    }

    if (pos3)
    {
        std::string szPos3 = args::get(pos3);
        if (szPos3 == "loop" || szPos3 == "true" || szPos3 == "1")
            m_bLoop = true;
    }

    if (m_szFilePath.empty())
    {
        PrintUsage(szProgramName, "Missing required argument: IP list file.");
        std::exit(-1);
    }

    if (m_iThreadCount <= 0)
    {
        PrintUsage(szProgramName, "Thread count must be greater than 0.");
        std::exit(-1);
    }

    LoadIps();
}

void SS::Config::LoadIps()
{
    FILE* pFile = fopen(m_szFilePath.c_str(), "r");
    if (!pFile)
    {
        SPDLOG_ERROR("No such file as {}.", m_szFilePath);
        std::exit(-1);
    }

    char szBuffer[512];
    while (fgets(szBuffer, sizeof(szBuffer), pFile) != NULL)
    {
        auto fnCheck = [](const char* str) -> bool
        {
            while (*str)
            {
                if (!isspace((unsigned char)*str))
                    return false;
                str++;
            }
            return true;
        };

        if (fnCheck(szBuffer))
            continue;

        std::string szClean = szBuffer;
        szClean.erase(0, szClean.find_first_not_of(" \t\n\r\f\v"));
        szClean.erase(szClean.find_last_not_of(" \t\n\r\f\v") + 1);

        if (szClean.empty())
            continue;

        std::string szIp = szClean;
        uint16_t iPort = m_iMinecraftPort;

        size_t iColonPos = szClean.find(':');
        if (iColonPos != std::string::npos)
        {
            szIp = szClean.substr(0, iColonPos);
            try
            {
                iPort = static_cast<uint16_t>(std::stoi(szClean.substr(iColonPos + 1)));
            }
            catch (...)
            {
                iPort = m_iMinecraftPort;
            }
        }

        Job::CJob job(szIp, iPort);
        Job::AddToQue(job);
        m_iTotalIpsLoaded++;
    }

    fclose(pFile);
    SPDLOG_INFO("IPs loaded: {}.", m_iTotalIpsLoaded);
}

void SS::Initialize(int argc, char** argv)
{
    g_pConfig = new SS::Config(argc, argv);
}

SS::Config* SS::GetConfig()
{
    return g_pConfig;
}