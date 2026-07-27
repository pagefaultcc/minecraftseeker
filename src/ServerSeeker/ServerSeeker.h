#pragma once

#include "../Logging/Logging.h"
#include "../Job/Job.h"

namespace SS
{
    class Config
    {
    public:
        Config(int argc, char** argv) : m_iArgc(argc), m_ppArgs(argv) 
        {
            if (m_iArgc < 3)
            {
                SPDLOG_ERROR("Wrong usage! expected: {} <thread_count> <file_name> [loop]", m_ppArgs[0]);
                std::exit(-1);
            }

            if (m_iArgc == 4)
                m_bLoop = true;

            m_iThreadCount = std::stoi(m_ppArgs[1]);

            FILE* pFile = fopen(m_ppArgs[2], "r");

            if (!pFile)
            {
                SPDLOG_ERROR("No such file as {}.", m_ppArgs[2]);
                std::exit(-1);
            }

            char buffer[256];
            while(fgets(buffer, sizeof(buffer), pFile) != NULL)
            {
                auto fnCheck = [](const char *str) -> bool
                {
                    while (*str)
                    {
                        if (!isspace((unsigned char)*str))
                            return false;

                        str++;
                    }

                    return true; 
                };

                if (fnCheck(buffer))
                    continue;

                Job::CJob job(buffer);
                Job::AddToQue(job);
            }

            SPDLOG_INFO("IP's loaded: {}.", Job::GetJobCount());
        }

        bool ShouldLoop() { return m_bLoop; }
        int  GetThreadCount() { return m_iThreadCount; }

    private:
        int     m_iArgc;
        char**  m_ppArgs;
            
        // Actual config
        bool        m_bLoop;
        int         m_iThreadCount;
        const char* m_szFilePath;
    };

    void    Initialize(int argc, char** argv);
    Config* GetConfig();
}
