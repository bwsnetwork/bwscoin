/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// These are the actors in the machine learning process.

#ifndef BWSCOIN_ACTOR_TYPE_H
#define BWSCOIN_ACTOR_TYPE_H

#include <string>

class CTransaction;

enum ActorType : unsigned int
{
    AT_Client = 0,
    AT_Miner,
    AT_Supervisor,
    AT_Evaluator,
    AT_Verifier,

    AT_COUNT
};

bool at_valid(const ActorType& actor);
bool at_valid(const int actor);
bool at_valid(const unsigned int actor);

ActorType at_actor(const CTransaction& tx);

ActorType at_from_string(const std::string& str);
std::string at_to_string(const ActorType& at);

#endif // BWSCOIN_ACTOR_TYPE_H
