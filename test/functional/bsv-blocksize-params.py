#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Prettywomancoin Core developers
# Copyright (c) 2017 The Prettywomancoin developers
# Copyright (c) 2019 Prettywomancoin Association
# Distributed under the Open PWC software license, see the accompanying file LICENSE.
"""
Test that the blockmaxsize and excessiveblocksize parameters are also
settable via the prettywomancoin.conf file.
"""

from test_framework.test_framework import PrettywomancoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.cdefs import (ONE_MEGABYTE)

import os

class PWCBlockSizeParams(PrettywomancoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1
        self.maxminedblocksize = 4 * ONE_MEGABYTE
        self.maxblocksize = 16 * ONE_MEGABYTE

    def setup_chain(self):
        super().setup_chain()
        with open(os.path.join(self.options.tmpdir + "/node0", "prettywomancoin.conf"), 'a', encoding='utf8') as f:
            f.write("blockmaxsize=" + str(self.maxminedblocksize) + "\n")
            f.write("excessiveblocksize=" + str(self.maxblocksize) + "\n")

    def add_options(self, parser):
        super().add_options(parser)

    def run_test(self):
        gires = self.nodes[0].getinfo()
        assert_equal(gires["maxblocksize"], self.maxblocksize)
        assert_equal(gires["maxminedblocksize"], self.maxminedblocksize)


if __name__ == '__main__':
    PWCBlockSizeParams().main()
