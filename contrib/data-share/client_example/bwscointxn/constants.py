"""
BWS Coin configuration constants
"""

# constants.py
#
# BWS Coin configuration constants
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

from platform import system


# RPC ports
BWSCOIN_IP = '127.0.0.1'
BWSCOIN_PORT_MAINNET = 8566
BWSCOIN_PORT_TESTNET = 18566

# REGTEST NOT SUPPORTED
# BWSCOIN_PORT_REGTEST = 19566

BWSCOIN_USER = ''
BWSCOIN_PASSWORD = ''

BWSCOIN_FEE = 0.0001
BWSCOIN_DUST = 0.00001

OP_RETURN_MAX = 80

MAX_BLOCKS = 10

NET_TIMEOUT = 10

# RANGES
SF8 = 1 << 8
SF16 = 1 << 16
SF32 = 1 << 32

os_type = system()

if os_type == 'Darwin':
    BWSCOIN_CONF_PATH = '/Library/Application Support/BWScoin/bitcoin.conf'
elif os_type == 'Linux':
    BWSCOIN_CONF_PATH = '/.bitcoin/bwscoin.conf'
else:
    BWSCOIN_CONF_PATH = ''
    raise OSError('Operating Sytem: {} not yet supported'.format(os_type))
