#include "API.h"

#include "../Logging/Logging.h"
#include "../Job/Job.h"
#include "../Worker/Worker.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void API::Heartbeat(const httplib::Request& req, httplib::Response& res)
{
    json response_json;
    response_json["status"] = "serverseeker is running.";

    res.set_content(response_json.dump(), "application/json");
    res.status = 200;
}

void API::Status(const httplib::Request& req, httplib::Response& res)
{
    json response_json;
    response_json["jobs"] = std::to_string(Job::GetJobCount());

    auto thread_array = json::array();
    
    auto threads = Worker::GetThreads();

    for (int i = 0; i < threads->size(); ++i)
    {
        auto cur_job = (*threads)[i].GetCurrentJob();
        thread_array.push_back(cur_job.IsValid() ? cur_job.GetIp() : "No job." );
    }

    response_json["threads"] = thread_array;

    res.set_content(response_json.dump(), "application/json");
    res.status = 200;
}

void API::StartServer()
{
    httplib::Server svr;

    svr.Get("/api/v1/heartbeat", Heartbeat);
    svr.Get("/api/v1/status", Status);

    SPDLOG_INFO("Starting API server at port 1337.");
    svr.listen("0.0.0.0", 1337);
}