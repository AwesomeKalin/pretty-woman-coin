#!/usr/bin/env python3
# Copyright (c) 2020 Prettywomancoin Association
# Distributed under the Open PWC software license, see the accompanying file LICENSE.
"""
Tests setting required parameters in prettywomancoin.conf configuration file.
"""

from test_framework.test_framework import PrettywomancoinTestFramework
from test_framework.cdefs import REGTEST_DEFAULT_MAX_BLOCK_SIZE
from test_framework.util import assert_equal

import os

class PWCRequiredParams(PrettywomancoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1

    def setup_chain(self):
        super().setup_chain()
        with open(os.path.join(self.options.tmpdir + "/node0", "prettywomancoin.conf"), 'a', encoding='utf8') as configFile:
            configFile.write("maxstackmemoryusageconsensus=0\n")
            configFile.write("excessiveblocksize=0\n")
            configFile.write("minminingtxfee=500\n")

    def add_options(self, parser):
        super().add_options(parser)

    def run_test(self):
        # We should be able to stop the running node. Test framework starts it by adding required parameters in the command line.
        self.stop_node(0)
        # If not set in the command line, check that required parameters can be set in the configuration file.
        self.runNodesWithRequiredParams = False
        self.start_node(0)
        nodeInfo = self.nodes[0].getinfo()
        # Check that unlimited value was set (excessiveblocksize=0)
        assert_equal(nodeInfo["maxblocksize"], REGTEST_DEFAULT_MAX_BLOCK_SIZE)

if __name__ == '__main__':
    PWCRequiredParams().main()
