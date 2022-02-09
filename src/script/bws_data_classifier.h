/* * Copyright (c) 2017-2020 Project PAI Foundation
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */

#ifndef BWS_DATA_CLASSIFIER_H
#define BWS_DATA_CLASSIFIER_H

// lists possible classes of data placed in structured OP_RETURN scripts;
// these values must not be changed (they are stored in scripts), so only appending is allowed

enum EDataClass
{
    CLASS_Staking,
    //CLASS_DataSharing,     // uncomment once it is ported to the structured OP_RETURN format

    NUM_DATA_CLASSES
};

#endif // BWS_DATA_CLASSIFIER_H
