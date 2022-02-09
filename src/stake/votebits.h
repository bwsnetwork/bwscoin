/* * Copyright (c) 2017-2020 Project PAI Foundation
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 */


#ifndef BWSCOIN_STAKE_VOTEBITS_H
#define BWSCOIN_STAKE_VOTEBITS_H

#include <cstdint>

// A generic vote bits implementation

// The feature list described in the enum can be extended as needed
// Specific functions are provided for regular transactions tree (RTT) management.
// The RTT is the tree of non-stake transactions, usually in the previous block.

class VoteBits {
public:
    // generic

    // the bit position for each named feature
    enum Feature : uint8_t {
        Rtt = 0,    // regular transactions tree
        Count = 16  // the maximum number of features / bits
    };

    // convenience value with all the bits set to zero, meaning
    // all the features are rejected
    static const VoteBits allRejected;

    // default constructor
    explicit VoteBits();

    // construction from explicit bits
    explicit VoteBits(const uint16_t& bits);

    // convenience constructor for feature initialization
    explicit VoteBits(const uint8_t pos, bool value);

    // copy constructor
    VoteBits(const VoteBits& o) = default;

    // move constructor
    VoteBits(VoteBits&& o) = default;

    // copy assignment operator
    VoteBits& operator=(const VoteBits& o);

    // move assignment operator
    VoteBits& operator=(VoteBits&& o);

    // equality operator
    bool operator==(const VoteBits& o) const;

    // inequality operator
    bool operator!=(const VoteBits& o) const;

    // get the actual bits word
    uint16_t getBits() const;

    // set the bit at the specified position
    void setBit(const uint8_t pos, bool value);

    // get the bit at the specified position
    bool getBit(const uint8_t pos) const;

    // regular transactions tree

    // convenience value to create RTT acceptance bits
    // please note that these contain a bit of 1 only for RTT,
    // the other being set to 0, thus rejected.
    static const VoteBits rttAccepted;

    // verify if the current vote bits approve the RTT
    bool isRttAccepted() const;

    // verify if the current vote bits approve the RTT
    void setRttAccepted(bool accepted = true);

    // serialization

    // serialize
    template<typename Stream>
    void Serialize(Stream& s) const
    {
        s << vb;
    }

    // deserialize
    template<typename Stream>
    void Unserialize(Stream& s)
    {
        s >> vb;
    }


private:
    uint16_t vb;
};

#endif //BWSCOIN_STAKE_VOTEBITS_H
