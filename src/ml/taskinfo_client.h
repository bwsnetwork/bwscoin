/* * Copyright (c) 2021 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#ifndef BWSCOIN_TASKINFO_CLIENT_H
#define BWSCOIN_TASKINFO_CLIENT_H

#include <string>

#include <univalue.h>

/**
 * Retrieve task informations from the server.
 */

class TaskInfoClient
{
public:

    static UniValue GetWaitingTasks(uint64_t page, uint64_t per_page);

    static UniValue GetStartedTasks(uint64_t page, uint64_t per_page);

    static UniValue GetCompletedTasks(uint64_t page, uint64_t per_page);

    static UniValue GetFailedTasks(uint64_t page, uint64_t per_page);

    static UniValue GetTaskDetails(const std::string& task_id);

    static std::string GetTaskId(const std::string& msg_id);

private:

    static UniValue GetTasks(std::string endpoint, uint64_t page, uint64_t per_page);
};

#endif // BWSCOIN_TASKINFO_CLIENT_H
