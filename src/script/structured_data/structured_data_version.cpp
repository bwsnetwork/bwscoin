/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "structured_data_version.h"

const StructuredDataVersion sdv_invalid = 0xFFFFFFFF;

const StructuredDataVersion sdv_current_version = 0;

bool sdv_valid(const StructuredDataVersion& data_version)
{
    return data_version != sdv_invalid && data_version <= sdv_current_version;
}
