# bws_test.py
#
# BWS Coin BWS Protocol test script
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

from pprint import pprint
from bwscointxn import BWS

bws = BWS(version=0x10)

pack0 = bws.pack('revoke', '', 'BWS', 'special')
print('Raw BWS pack[0]: {}'.format(pack0))
print('Assembled BWS pack[0]:')
print(bws)

pack1 = bws.pack('grant', '', 'BWS', 'optimal')
print('Raw BWS pack[1]: {}'.format(pack1))
print('Assembled BWS pack[1]:')
print(bws)

res = bws.unpack(pack1)
print('BWS unpack pack[1] result:')
pprint(res)
print('\nDisassembled BWS pack[1]:')
print(bws)

res = bws.unpack(pack0)
print('BWS unpack pack[0] result:')
pprint(res)
print('\nDisassembled BWS pack[0]:')
print(bws)

pack3 = BWS.assembly('grant', '', 'BWS', 'train behaviour', version=0x10)
print('Raw BWS pack[3]: {}'.format(pack3))
print('Assembled BWS pack[3]:')

res = BWS.disassembly(pack3, version=0x10)
print('BWS unpack pack[3] result:')
pprint(res)
