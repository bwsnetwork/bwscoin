/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// Structured data script version

#ifndef BWSCOIN_STRUCTURED_DATA_VERSION_H
#define BWSCOIN_STRUCTURED_DATA_VERSION_H

typedef unsigned int StructuredDataVersion;

// the invalid version placeholder (0)
extern const StructuredDataVersion sdv_invalid;

// the version to be used by default for new scripts
// (should be monotonic)
extern const StructuredDataVersion sdv_current_version;

bool sdv_valid(const StructuredDataVersion& data_version);

#endif // BWSCOIN_STRUCTURED_DATA_VERSION_H
