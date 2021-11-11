# retrieve_bwscoin.py
# 
# CLI wrapper for retrieving BWS Coin transaction with OP_RETURN metadata
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

from re import sub
from sys import argv, exit
from bwscointxn import Transaction, bin2hex


usage_string = '''Usage: python retrieve_bwscoin.py <ref> <testnet (opt)>'''

if len(argv) < 2:
    exit(usage_string)

ref = argv[1]

if len(argv) > 2:
    testnet = bool(argv[2])
else:
    testnet = False

bwscoin_txn = Transaction(testnet)
results = bwscoin_txn.retrieve(ref, 1)

# Equivalent with:
# res = Transaction.bwscoin_retrieve(ref, 1, testnet)

if 'error' in results:
    print('BWScoinTransactionError: {}\n'.format(results['error']))

elif len(results):
    for res in results:
        data = res['data']
        data_ascii = sub(b'[^\x20-\x7E]', b'?', data).decode('utf-8')
        txids = res['txids']
        heights = '\n'.join(map(str, res['heights']))
        blocks = ('\n{}\n'.format(heights)).replace('\n0\n', '\n[mempool]\n')

        print('Hex: ({} bytes)\n{}\n'.format(str(len(data)), bin2hex(data)))
        print('ASCII:\n{}\n'.format(data_ascii))
        print('TxIDs: (count {})\n{}\n'.format(str(len(txids)), ''.join(txids)))
        print('Blocks:{}'.format(blocks))

        if 'ref' in res:
            print('Best ref: {}\n'.format(res['ref']))

        if 'error' in res:
            print('BWScoinTransactionError: {}\n'.format(res['error'] + "\n"))
else:
    print('No matching data was found')
