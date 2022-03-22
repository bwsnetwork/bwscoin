/* * Copyright (c) 2022 Valdi Labs
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

// lists possible classes of data placed in structured OP_RETURN scripts;
// these values must not be changed (they are stored in scripts), so only appending is allowed

#ifndef BWSCOIN_STRUCTURED_DATA_CLASS_H
#define BWSCOIN_STRUCTURED_DATA_CLASS_H

enum StructuredDataClass : unsigned int
{
    SDC_PoUW = 0,
    //SDC_DataSharing,

    SDC_COUNT
};

bool sdc_valid(const StructuredDataClass& data_class);
bool sdc_valid(const int data_class);

#endif // BWSCOIN_STRUCTURED_DATA_CLASS_H
