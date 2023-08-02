Sample init scripts and service configuration for prettywomancoind
==========================================================

Sample scripts and configuration files for systemd, Upstart and OpenRC
can be found in the contrib/init folder.

    contrib/init/prettywomancoind.service:    systemd service unit configuration
    contrib/init/prettywomancoind.openrc:     OpenRC compatible SysV style init script
    contrib/init/prettywomancoind.openrcconf: OpenRC conf.d file
    contrib/init/prettywomancoind.conf:       Upstart service configuration file
    contrib/init/prettywomancoind.init:       CentOS compatible SysV style init script

1. Service User
---------------------------------

All three Linux startup configurations assume the existence of a "prettywomancoin" user
and group.  They must be created before attempting to use these scripts.
The OS X configuration assumes prettywomancoind will be set up for the current user.

2. Configuration
---------------------------------

At a bare minimum, prettywomancoind requires that the rpcpassword setting be set
when running as a daemon.  If the configuration file does not exist or this
setting is not set, prettywomancoind will shutdown promptly after startup.

This password does not have to be remembered or typed as it is mostly used
as a fixed token that prettywomancoind and client programs read from the configuration
file, however it is recommended that a strong and secure password be used
as this password is security critical to securing the wallet should the
wallet be enabled.

If prettywomancoind is run with the "-server" flag (set by default), and no rpcpassword is set,
it will use a special cookie file for authentication. The cookie is generated with random
content when the daemon starts, and deleted when it exits. Read access to this file
controls who can access it through RPC.

By default the cookie is stored in the data directory, but it's location can be overridden
with the option '-rpccookiefile'.

This allows for running prettywomancoind without having to do any manual configuration.

`conf`, `pid`, and `wallet` accept relative paths which are interpreted as
relative to the data directory. `wallet` *only* supports relative paths.

For an example configuration file that describes the configuration settings,
see `contrib/debian/examples/prettywomancoin.conf`.

3. Paths
---------------------------------

All three configurations assume several paths that might need to be adjusted.

Binary:              `/usr/bin/prettywomancoind`  
Configuration file:  `/etc/prettywomancoin/prettywomancoin.conf`  
Data directory:      `/var/lib/prettywomancoind`  
PID file:            `/var/run/prettywomancoind/prettywomancoind.pid` (OpenRC and Upstart) or `/var/lib/prettywomancoind/prettywomancoind.pid` (systemd)  
Lock file:           `/var/lock/subsys/prettywomancoind` (CentOS)  

The configuration file, PID directory (if applicable) and data directory
should all be owned by the prettywomancoin user and group.  It is advised for security
reasons to make the configuration file and data directory only readable by the
prettywomancoin user and group.  Access to prettywomancoin-cli and other prettywomancoind rpc clients
can then be controlled by group membership.

4. Installing Service Configuration
-----------------------------------

4a) systemd

Installing this .service file consists of just copying it to
/usr/lib/systemd/system directory, followed by the command
`systemctl daemon-reload` in order to update running systemd configuration.

To test, run `systemctl start prettywomancoind` and to enable for system startup run
`systemctl enable prettywomancoind`

4b) OpenRC

Rename prettywomancoind.openrc to prettywomancoind and drop it in /etc/init.d.  Double
check ownership and permissions and make it executable.  Test it with
`/etc/init.d/prettywomancoind start` and configure it to run on startup with
`rc-update add prettywomancoind`

4c) Upstart (for Debian/Ubuntu based distributions)

Drop prettywomancoind.conf in /etc/init.  Test by running `service prettywomancoind start`
it will automatically start on reboot.

NOTE: This script is incompatible with CentOS 5 and Amazon Linux 2014 as they
use old versions of Upstart and do not supply the start-stop-daemon utility.

4d) CentOS

Copy prettywomancoind.init to /etc/init.d/prettywomancoind. Test by running `service prettywomancoind start`.

Using this script, you can adjust the path and flags to the prettywomancoind program by
setting the BITCOIND and FLAGS environment variables in the file
/etc/sysconfig/prettywomancoind. You can also use the DAEMONOPTS environment variable here.

5. Auto-respawn
-----------------------------------

Auto respawning is currently only configured for Upstart and systemd.
Reasonable defaults have been chosen but YMMV.
