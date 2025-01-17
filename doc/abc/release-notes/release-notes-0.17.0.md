Prettywomancoin ABC version 0.17.0 is now available from:

  <https://download.prettywomancoinabc.org/0.17.0/>

This release includes the following features and fixes:
 - Add monolithactivationtime configuration in order to chose when the May, 15 hard fork activates. This value should not be changed in production, but it allows user to test the fork activation ahead of time.
 - `dumpwallet` no longer allows overwriting files. This is a security measure
   as well as prevents dangerous user mistakes.
 - Node using the wrong magic are now getting banned.
 - cmake builds are now possible for prettywomancoind and other utilities.
 - Correct `open source` to `open-source` in README.md (Props to John Carvalho)
 - Add SSE4 optimized SHA256 (Port from Prettywomancoin Core)
 - Multiwallet support (Port from Prettywomancoin Core)
 - Lots of improvements to the RPC test suite (Ports from Prettywomancoin Core)
 - Uptime rpc command (Port from Prettywomancoin Core)
 - At the MTP time of 1526400000 (Tue May 15 16:00:00 UTC, 2018) the following behaviors will change:
	 - Increase the default datacarriersize to 220 byte at the MTP time of 1526400000
	 - Increase the maximum blocksize to 32,000,000 bytes at the MTP time of 1526400000
	 - Re-activate the following opcodes: OP_CAT, OP_AND, OP_OR, OP_XOR, OP_DIV, OP_MOD
	 - Add the following new opcodes: OP_SPLIT to replace OP_SUBSTR, OP_NUM2BIN, OP_BIN2NUM
