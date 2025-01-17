# Pretty Woman Coin version 0.1.1 Release Notes

## List of Changes
* added `broadcastdelay` configuration parameter. Sets the number of milliseconds
    to wait before relaying transactions. When set, this replaces the random delay.
* updated DNS seeds to point to Pretty Woman Coin dedicated seed nodes
* optimized handling of P2P inventory messages into own threads, increased parallelization of tx propagation
* increased maxmimum P2P protocol message length
* set maximum number of items to include in inventory size to be dependent on block size
* added the Scaling Test Network
  * activate with `stn=1` in prettywomancoin.conf
  * getinfo returns new field, `stn=true|false`
* renamed `debug.log` to `prettywomancoind.log`
* added capability to log source of transactions  
* various other fixes

## Scaling Test Network Reset
The Scaling Test Network has been reset to block height 2300 on 31st of January 2019.
 