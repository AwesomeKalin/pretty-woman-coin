#!/usr/bin/env python3
# Copyright (c) 2019 Prettywomancoin Association
# Distributed under the Open PWC software license, see the accompanying file LICENSE.

from genesis_upgrade_tests import tests
from test_framework.height_based_test_framework import SimplifiedTestFramework

if __name__ == "__main__":
    t = SimplifiedTestFramework([t() for t in tests()])
    t.main()