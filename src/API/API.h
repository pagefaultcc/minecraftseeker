#pragma once

#include <cstdint>
#include <httplib.h>

namespace API
{
    void Heartbeat(const httplib::Request& req, httplib::Response& res);
    void Status(const httplib::Request& req, httplib::Response& res);
    void Metrics(const httplib::Request& req, httplib::Response& res);
    void Hits(const httplib::Request& req, httplib::Response& res);

    void StartServer(uint16_t iPort);
    void StopServer();
}
