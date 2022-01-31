/* * Copyright (c) 2021 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "taskinfo_client.h"

#include "httpclient.h"
#include "util.h"

UniValue TaskInfoClient::GetWaitingTasks(uint64_t page, uint64_t per_page)
{
    return TaskInfoClient::GetTasks("waiting", page, per_page);
}

UniValue TaskInfoClient::GetStartedTasks(uint64_t page, uint64_t per_page)
{
    return TaskInfoClient::GetTasks("started", page, per_page);
}

UniValue TaskInfoClient::GetCompletedTasks(uint64_t page, uint64_t per_page)
{
    return TaskInfoClient::GetTasks("completed", page, per_page);
}

UniValue TaskInfoClient::GetFailedTasks(uint64_t page, uint64_t per_page)
{
    return TaskInfoClient::GetTasks("failed", page, per_page);
}

UniValue TaskInfoClient::GetTaskDetails(const std::string& task_id)
{
    std::string verificationServerAddress = gArgs.GetArg("-verificationserver", "localhost:50011");

    UniValue body(UniValue::VOBJ);
    body.pushKV("task_id", task_id);

    HttpClient client(verificationServerAddress);
    auto response = client.post("/taskinfo/taskdetails/", body);

    return response.body;
}

std::string TaskInfoClient::GetTaskId(const std::string& msg_id)
{
    std::string verificationServerAddress = gArgs.GetArg("-verificationserver", "localhost:50011");
    std::string endpoint = "/messages/";
    endpoint += msg_id;
    endpoint += "/task_id";

    HttpClient client(verificationServerAddress);
    auto response = client.post(endpoint, UniValue());

    if (response.status == HttpResponse::Failed)
        return "unavailable";

    auto task_id = response.body["task_id"];
    if (!task_id.isStr())
        return "unavailable";

    return task_id.get_str();
}

UniValue TaskInfoClient::GetTasks(std::string state, uint64_t page, uint64_t per_page)
{
    std::string verificationServerAddress = gArgs.GetArg("-verificationserver", "localhost:50011");
    std::string endpoint = "/tasks";

    UniValue query_params(UniValue::VOBJ);
    query_params.pushKV("task_state", state);
    query_params.pushKV("page", page);
    query_params.pushKV("per_page", per_page);

    HttpClient client(verificationServerAddress);
    auto response = client.get(endpoint, query_params);

    if (response.status == 1) {
        return response.body;
    }
    char buffer[2048];
    sprintf(buffer, "Request to server: %s endpoint: %s failed with message: %s and code: %d",
                   verificationServerAddress.c_str(), endpoint.c_str(), response.message.c_str(), response.http_code);
    return buffer;
}
