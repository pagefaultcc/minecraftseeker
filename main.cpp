#include <iostream>
#include <string>

#include "src/Database/Database.h"
#include "src/Logging/Logging.h"
#include "src/Minecraft/Minecraft.h"
#include "src/Job/Job.h"
#include "src/Worker/Worker.h"
#include "src/API/API.h"

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cout << "Wrong usage! Example: " << argv[0] << " <thread_count> <file>" << "\n";
        return -1;
    }

    Logging::Initialize();
    Database::Initialize();

    Worker::Initialize(std::stoi(argv[1]));

    FILE* pFile = fopen(argv[2], "r");

    if (!pFile)
    {
        SPDLOG_ERROR("No such file as {}.", argv[2]);
        return -1;
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

    Worker::Start();

    API::StartServer();

    return 0;
}