/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "actor_type.h"

bool at_valid(const ActorType& actor)
{
    return actor < AT_COUNT;
}

bool at_valid(const int actor)
{
    return actor >= 0 && actor < static_cast<int>(AT_COUNT);
}

ActorType at_from_string(const std::string& str)
{
    std::string lstr;
    std::transform(str.begin(), str.end(), std::back_inserter(lstr), std::tolower);

    if (lstr == "client")
        return AT_Client;
    else if (lstr == "miner")
        return AT_Miner;

    return AT_COUNT;
}

std::string at_to_string(const ActorType& at)
{
    switch (at) {
    case AT_Client: return "client"; break;
    case AT_Miner: return "miner"; break;
    default: return ""; break;
    }
}
