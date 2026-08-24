#include "API.h"

#include "../Logging/Logging.h"
#include "../Job/Job.h"
#include "../Worker/Worker.h"
#include "../ServerSeeker/ServerSeeker.h"
#include "../../CONFIGURATION.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static httplib::Server g_HttpServer;

static void SetJsonResponse(httplib::Response& res, const json& jData, int iStatusCode = 200)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "*");
    res.set_content(jData.dump(), "application/json");
    res.status = iStatusCode;
}

void API::Heartbeat(const httplib::Request& req, httplib::Response& res)
{
    long long iNow = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    long long iStartTime = Worker::GetMetrics()->m_iStartTime;
    long long iUptime = (iStartTime > 0) ? (iNow - iStartTime) : 0;

    json resJson;
    resJson["status"] = "ok";
    resJson["service"] = "serverseeker";
    resJson["version"] = SERVERSEEKER_VERSION;
    resJson["uptime_seconds"] = iUptime;

    SetJsonResponse(res, resJson);
}

void API::Status(const httplib::Request& req, httplib::Response& res)
{
    auto pMetrics = Worker::GetMetrics();
    long long iNow = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    long long iElapsed = std::max(1LL, iNow - pMetrics->m_iStartTime);
    long long iPinged = pMetrics->m_iServerPinged.load();
    long long iServersFound = pMetrics->m_iServersFound.load();
    long long iPlayersFound = pMetrics->m_iPlayersFound.load();
    double flRate = static_cast<double>(iPinged) / static_cast<double>(iElapsed);

    int iTotal = SS::GetConfig()->GetTotalIpsLoaded();
    int iRemaining = Job::GetJobCount();
    double flPercent = iTotal > 0 ? (static_cast<double>(iPinged) * 100.0 / static_cast<double>(iTotal)) : 0.0;
    if (flPercent > 100.0)
        flPercent = 100.0;

    auto threadArray = json::array();
    auto pThreads = Worker::GetThreads();
    for (size_t i = 0; i < pThreads->size(); ++i)
    {
        auto curJob = (*pThreads)[i].GetCurrentJob();
        threadArray.push_back(curJob.IsValid() ? (curJob.GetIp() + ":" + std::to_string(curJob.GetPort())) : "idle");
    }

    json resJson;
    resJson["status"] = "running";
    resJson["version"] = SERVERSEEKER_VERSION;
    resJson["uptime_seconds"] = iElapsed;
    resJson["thread_count"] = SS::GetConfig()->GetThreadCount();
    resJson["active_threads"] = Worker::GetActiveThreadCount();
    resJson["minecraft_port"] = SS::GetConfig()->GetMinecraftPort();
    resJson["monitoring_port"] = SS::GetConfig()->GetMonitoringPort();
    resJson["loop"] = SS::GetConfig()->ShouldLoop();
    resJson["total_jobs"] = iTotal;
    resJson["remaining_jobs"] = iRemaining;
    resJson["servers_pinged"] = iPinged;
    resJson["servers_found"] = iServersFound;
    resJson["players_found"] = iPlayersFound;
    resJson["rate_pings_per_sec"] = flRate;
    resJson["progress_percent"] = flPercent;
    resJson["threads"] = threadArray;

    SetJsonResponse(res, resJson);
}

void API::Metrics(const httplib::Request& req, httplib::Response& res)
{
    auto pMetrics = Worker::GetMetrics();
    long long iNow = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    long long iElapsed = std::max(1LL, iNow - pMetrics->m_iStartTime);
    long long iPinged = pMetrics->m_iServerPinged.load();

    json resJson;
    resJson["uptime_seconds"] = iElapsed;
    resJson["servers_pinged"] = iPinged;
    resJson["servers_found"] = pMetrics->m_iServersFound.load();
    resJson["players_found"] = pMetrics->m_iPlayersFound.load();
    resJson["rate_pings_per_sec"] = static_cast<double>(iPinged) / static_cast<double>(iElapsed);
    resJson["active_threads"] = Worker::GetActiveThreadCount();
    resJson["queue_length"] = Job::GetJobCount();

    SetJsonResponse(res, resJson);
}

void API::Hits(const httplib::Request& req, httplib::Response& res)
{
    auto vecHits = Worker::GetRecentHits();
    auto hitsArray = json::array();

    for (const auto& hit : vecHits)
    {
        json hitObj;
        hitObj["ip"] = hit.szIp;
        hitObj["port"] = hit.iPort;
        hitObj["players"] = hit.vecPlayers;
        hitObj["player_count"] = hit.vecPlayers.size();
        hitObj["timestamp"] = hit.iTimestamp;
        hitsArray.push_back(hitObj);
    }

    json resJson;
    resJson["total_recent_hits"] = hitsArray.size();
    resJson["hits"] = hitsArray;

    SetJsonResponse(res, resJson);
}

void API::StartServer(uint16_t iPort)
{
    if (iPort == 0)
        return;

    g_HttpServer.Get("/api/v1/heartbeat", Heartbeat);
    g_HttpServer.Get("/api/v1/status", Status);
    g_HttpServer.Get("/api/v1/metrics", Metrics);
    g_HttpServer.Get("/api/v1/hits", Hits);

    SPDLOG_INFO("Starting API server on port {}.", iPort);
    g_HttpServer.listen("0.0.0.0", iPort);
}

void API::StopServer()
{
    g_HttpServer.stop();
}