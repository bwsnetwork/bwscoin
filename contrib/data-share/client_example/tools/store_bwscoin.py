# store_bwscoin.py
# 
# CLI wrapper for storing BWS Coin transaction with OP_RETURN metadata
#
# Copyright (c) 2021 Valdi - https://valdi.ai
# Copyright (c) ObEN, Inc. - https://oben.me/
# Copyright (c) Coin Sciences Ltd
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

from sys import argv, exit
from bwscointxn import Transaction, hex2bin


usage_string = '''Usage: python store_bwscoin.py <data> <testnet (opt)>'''

if len(argv) < 2:
    exit(usage_string)

data = argv[1]

if len(argv) > 2:
    testnet = bool(argv[2])
else:
    testnet = False

data_from_hex = hex2bin(data)

if data_from_hex is not None:
    data = data_from_hex

bwscoin_txn = Transaction(testnet)
res = bwscoin_txn.store(data)

# Equivalent with:
# res = Transaction.bwscoin_store(data, testnet)

if 'error' in res:
    print('BWScoinTransactionError: {}\n'.format(res['error']))
else:
    print('TxIDs:\n\n{}\n\nRef: {}\n'.format(''.join(res['txids']), res['ref']))
