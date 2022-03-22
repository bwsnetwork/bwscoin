/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#include "structured_data_class.h"

bool sdc_valid(const StructuredDataClass& data_class)
{
    return data_class < SDC_COUNT;
}

bool sdc_valid(const int data_class)
{
    return data_class >= 0 && data_class < static_cast<int>(SDC_COUNT);
}
