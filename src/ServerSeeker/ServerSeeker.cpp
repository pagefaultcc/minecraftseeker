#include "ServerSeeker.h"

SS::Config* g_Config;

void SS::Initialize(int argc, char** argv)
{
    g_Config = new SS::Config(argc, argv);
}

SS::Config* SS::GetConfig()
{
    return g_Config;
}