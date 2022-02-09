# bws_transaction_operations.py
#
# BWS Coin Send/Store/Retrieve BWS Protocol in blockchain test script
#
# Copyright (c) 2021 Valdi - https://valdi.ai
# Copyright (c) ObEN, Inc. - https://oben.me/
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

from time import sleep
from pprint import pprint
from bwscointxn import BWS, Transaction


address = 'MqUSBmN24cvWh2AKnzg8aj59snJxd8hdjU'
amount = 0.0001

BWS = BWS(version=0x10)

bws_pack = BWS.pack('revoke', '', 'BWS', 'special')
print('Raw BWS bws_pack: {}'.format(bws_pack))
print('Assembled BWS bws_pack:')
print(BWS)

bwscoin_txn = Transaction(testnet=True)
res = bwscoin_txn.send(address, amount, bws_pack)

if 'error' in res:
    print('BWS Coin::Send TransactionError: {}\n'.format(res['error']))
else:
    print('BWS Coin::Send TxID: {}\n'.format(res['txid']))

timeout = 20

print 'Wait {} seconds to make sure transaction is sent...'.format(timeout)

sleep(timeout)


res = bwscoin_txn.store(bws_pack)

bws_reference = None
if 'error' in res:
    print('BWS Coin::Store TransactionError: {}\n'.format(res['error']))
else:
    bws_reference = res['ref']
    print('TxIDs:\n\n{}\n\nRef: {}\n'.format(''.join(res['txids']), res['ref']))

if bws_reference:
    res = bwscoin_txn.retrieve(bws_reference, 1)
    if 'error' in res:
        print('BWS Coin::Retrieve TransactionError: {}\n'.format(res['error']))

    elif len(res):
        for r in res:
            data = r['data']
            bws_unpack = BWS.unpack(data)
            print('BWS unpack pack[1] result:')
            pprint(bws_unpack)
            print('\nDisassembled BWS pack[1]:')
            print(BWS)
