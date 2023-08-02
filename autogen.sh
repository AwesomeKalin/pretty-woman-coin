#!/bin/sh
# Copyright (c) 2013-2016 The Prettywomancoin Core developers
# Copyright (c) 2019 Prettywomancoin Association
# Distributed under the Open PWC software license, see the accompanying file LICENSE.

set -e
srcdir="$(dirname $0)"
cd "$srcdir"
if [ -z ${LIBTOOLIZE} ] && GLIBTOOLIZE="`which glibtoolize 2>/dev/null`"; then
  LIBTOOLIZE="${GLIBTOOLIZE}"
  export LIBTOOLIZE
fi
which autoreconf >/dev/null || \
  (echo "configuration failed, please install autoconf first" && exit 1)
autoreconf --install --force --warnings=all
