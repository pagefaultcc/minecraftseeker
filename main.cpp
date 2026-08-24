#include "src/Database/Database.h"
#include "src/Logging/Logging.h"
#include "src/Minecraft/Minecraft.h"
#include "src/Job/Job.h"
#include "src/Worker/Worker.h"
#include "src/API/API.h"
#include "src/Output/Output.h"
#include "src/ServerSeeker/ServerSeeker.h"

int main(int argc, char** argv)
{
    Logging::Initialize();

    SS::Initialize(argc, argv);

    Output::Initialize(SS::GetConfig()->GetOutputFile());
    Database::Initialize(SS::GetConfig()->GetDatabaseUri());
    Worker::Initialize(SS::GetConfig()->GetThreadCount());

    Worker::Start();
    API::StartServer(SS::GetConfig()->GetMonitoringPort());

    return 0;
}