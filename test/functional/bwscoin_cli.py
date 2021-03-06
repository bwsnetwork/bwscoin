#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test bwscoin-cli"""
from test_framework.test_framework import BWScoinTestFramework
from test_framework.util import assert_equal, assert_raises_process_error, get_auth_cookie

class TestBWScoinCli(BWScoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        """Main test logic"""

        self.log.info("Compare responses from gewalletinfo RPC and `bwscoin-cli getwalletinfo`")
        cli_response = self.nodes[0].cli.getwalletinfo()
        rpc_response = self.nodes[0].getwalletinfo()
        assert_equal(cli_response, rpc_response)

        self.log.info("Compare responses from getblockchaininfo RPC and `bwscoin-cli getblockchaininfo`")
        cli_response = self.nodes[0].cli.getblockchaininfo()
        rpc_response = self.nodes[0].getblockchaininfo()
        assert_equal(cli_response, rpc_response)

        user, password = get_auth_cookie(self.nodes[0].datadir)

        self.log.info("Test -stdinrpcpass option")
        assert_equal(0, self.nodes[0].cli('-rpcuser=%s' % user, '-stdinrpcpass', input=password).getblockcount())
        assert_raises_process_error(1, "incorrect rpcuser or rpcpassword", self.nodes[0].cli('-rpcuser=%s' % user, '-stdinrpcpass', input="foo").echo)

        self.log.info("Test -stdin and -stdinrpcpass")
        assert_equal(["foo", "bar"], self.nodes[0].cli('-rpcuser=%s' % user, '-stdin', '-stdinrpcpass', input=password + "\nfoo\nbar").echo())
        assert_raises_process_error(1, "incorrect rpcuser or rpcpassword", self.nodes[0].cli('-rpcuser=%s' % user, '-stdin', '-stdinrpcpass', input="foo").echo)

if __name__ == '__main__':
    TestBWScoinCli().main()
