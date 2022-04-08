/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// Transaction size helpers.

#ifndef BWSCOIN_ML_TX_SIZE_H
#define BWSCOIN_ML_TX_SIZE_H

#include <cstddef>
#include <vector>
#include <json/nlohmann/json.hpp>

// regular transactions
size_t p2pkh_txin_estimated_size(bool compressed = true);
size_t p2pkh_txout_estimated_size();

// ml transactions special outputs
size_t byt_txout_estimated_size(const nlohmann::json& payload);
size_t rvt_txout_estimated_size();
std::vector<size_t> pft_txout_estimated_sizes(const nlohmann::json& task);
size_t jnt_txout_estimated_size();

// ml transactions
size_t byt_estimated_size(const unsigned long txin_count, const nlohmann::json& payload,
                          const bool has_change = true, const bool include_expiry = true);
size_t rvt_estimated_size(const bool include_expiry = true);
size_t pft_estimated_size(const unsigned long extra_funding_count, const nlohmann::json& task,
                          const bool has_change = true, const bool include_expiry = true);
size_t jnt_estimated_size(const bool include_expiry = true);

#endif // BWSCOIN_ML_TX_SIZE_H
