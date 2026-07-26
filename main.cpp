#include <iostream>
#include <string>

#include "src/Database/Database.h"
#include "src/Logging/Logging.h"
#include "src/Minecraft/Minecraft.h"
#include "src/Job/Job.h"
#include "src/Worker/Worker.h"
#include "src/API/API.h"
#include "src/ServerSeeker/ServerSeeker.h"

int main(int argc, char** argv)
{
    Logging::Initialize();

    SS::Initialize(argc, argv);

    Database::Initialize();
    Worker::Initialize(SS::GetConfig()->GetThreadCount());

    Worker::Start();
    API::StartServer(); // would run indefinetly

    return 0;
}