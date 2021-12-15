/* * Copyright (c) 2017-2020 Project PAI Foundation
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#ifndef BWSCOIN_STAKE_VALUE_H
#define BWSCOIN_STAKE_VALUE_H

#include <stdint.h>

struct Value final
{
    Value(uint32_t height, bool missed, bool revoked, bool spent, bool expired);
    explicit Value(uint32_t height);
    Value(const Value&) = default;
    Value(Value&&) = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) = default;

    friend bool operator==(const Value&, const Value&);

    uint32_t height; // Height is the block height of the associated ticket.
    bool missed; // Flags defining the ticket state.
    bool revoked;
    bool spent;
    bool expired;
};

#endif // BWSCOIN_STAKE_VALUE_H
