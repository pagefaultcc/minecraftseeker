#pragma once

#include "httplib.h"

namespace API
{
    // /api/v1/heartbeat
    void Heartbeat(const httplib::Request& req, httplib::Response& res);

    // /api/v1/status
    void Status(const httplib::Request& req, httplib::Response& res);

    void StartServer();
}
