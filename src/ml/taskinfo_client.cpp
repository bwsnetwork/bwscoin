/* * Copyright (c) 2021 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "taskinfo_client.h"

#include "httpclient.h"
#include "util.h"

UniValue TaskInfoClient::GetWaitingTasks(uint64_t page, uint64_t per_page)
{
    return TaskInfoClient::GetTasks("/taskinfo/waitingtasks/", page, per_page);
}

UniValue TaskInfoClient::GetStartedTasks(uint64_t page, uint64_t per_page)
{
    return TaskInfoClient::GetTasks("/taskinfo/startedtasks/", page, per_page);
}

UniValue TaskInfoClient::GetCompletedTasks(uint64_t page, uint64_t per_page)
{
    return TaskInfoClient::GetTasks("/taskinfo/completedtasks/", page, per_page);
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

    UniValue body(UniValue::VOBJ);
    body.pushKV("msg_id", msg_id);

    HttpClient client(verificationServerAddress);
    auto response = client.post("/taskinfo/taskid/", body);

    if (response.status == HttpResponse::Failed)
        return "unavailable";

    auto task_id = response.body["task_id"];
    if (!task_id.isStr())
        return "unavailable";

    return task_id.get_str();
}

UniValue TaskInfoClient::GetTasks(std::string endpoint, uint64_t page, uint64_t per_page)
{
    std::string verificationServerAddress = gArgs.GetArg("-verificationserver", "localhost:50011");

    UniValue body(UniValue::VOBJ);
    body.pushKV("page", page);
    body.pushKV("per_page", per_page);

    HttpClient client(verificationServerAddress);
    auto response = client.post(endpoint, body);

    return response.body;
}
