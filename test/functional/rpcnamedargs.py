#!/usr/bin/env python3
# Copyright (c) 2016 The Prettywomancoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from decimal import Decimal

from test_framework.test_framework import PrettywomancoinTestFramework
from test_framework.authproxy import JSONRPCException
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    assert_is_hex_string,
    assert_is_hash_string,
)


class NamedArgumentTest(PrettywomancoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        h = node.help(command='getinfo')
        assert(h.startswith('getinfo\n'))

        assert_raises_rpc_error(-8, 'Unknown named parameter',
                                node.help, random='getinfo')

        h = node.getblockhash(height=0)
        node.getblock(blockhash=h)

        assert_equal(node.echo(), [])
        assert_equal(node.echo(arg0=0, arg9=9), [0] + [None] * 8 + [9])
        assert_equal(node.echo(arg1=1), [None, 1])
        assert_equal(node.echo(arg9=None), [None] * 10)
        assert_equal(node.echo(arg0=0, arg3=3, arg9=9),
                     [0] + [None] * 2 + [3] + [None] * 5 + [9])


if __name__ == '__main__':
    NamedArgumentTest().main()
