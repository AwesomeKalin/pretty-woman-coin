#!/usr/bin/env python3
# Copyright (c) 2017 The Prettywomancoin Core developers
# Copyright (c) 2019 Prettywomancoin Association
# Distributed under the Open PWC software license, see the accompanying file LICENSE.
"""Class for prettywomancoind node under test"""

import decimal
import errno
import http.client
import json
import logging
import os
import subprocess
import time
import atexit

from .mininode import COIN, ToHex
from .util import (
    assert_equal,
    get_rpc_proxy,
    rpc_url,
    wait_until,
)
from .authproxy import JSONRPCException

BITCOIND_PROC_WAIT_TIMEOUT = 120

# Keep a global list of started external processes so that they can be killed
# before Python exits if they are still running.
TestNode_process_list = []


class TestNode():
    """A class for representing a prettywomancoind node under test.

    This class contains:

    - state about the node (whether it's running, etc)
    - a Python subprocess.Popen object representing the running process
    - an RPC connection to the node

    To make things easier for the test writer, a bit of magic is happening under the covers.
    Any unrecognised messages will be dispatched to the RPC connection."""

    def __init__(self, i, dirname, extra_args, rpchost, timewait, binary, stderr, mocktime, coverage_dir):
        self.index = i
        self.datadir = os.path.join(dirname, "node" + str(i))
        self.rpchost = rpchost
        if timewait:
            self.rpc_timeout = timewait
        else:
            # Wait for up to 60 seconds for the RPC server to respond
            self.rpc_timeout = 60
        if binary is None:
            self.binary = [os.getenv("BITCOIND", "prettywomancoind")]
        else:
            self.binary = binary
        self.stderr = stderr
        self.coverage_dir = coverage_dir
        # Most callers will just need to add extra args to the standard list below. For those callers that need more flexibity, they can just set the args property directly.
        self.extra_args = extra_args
        self.args = self.binary + ["-datadir=" + self.datadir, "-server", "-keypool=1", "-discover=0", "-rest", "-logtimemicros",
                     "-debug", "-debugexclude=libevent", "-debugexclude=leveldb", "-mocktime=" + str(mocktime), "-uacomment=testnode%d" % i]

        self.cli = TestNodeCLI(
            os.getenv("BITCOINCLI", "prettywomancoin-cli"), self.datadir)

        self.running = False
        self.process = None
        self.rpc_connected = False
        self.rpc = None
        self.url = None
        self.relay_fee_cache = None
        self.log = logging.getLogger('TestFramework.node%d' % i)

    def __getattr__(self, *args, **kwargs):
        """Dispatches any unrecognised messages to the RPC connection."""
        assert self.rpc_connected and self.rpc is not None, "Error: no RPC connection"
        return self.rpc.__getattr__(*args, **kwargs)

    def setRequiredArgs(self, inputArgs, runNodesWithRequiredParams):
        """Sets the following required prettywomancoind arguments with default values if they are not already set.
           This prevents node from failing in functional tests
        """
        if not runNodesWithRequiredParams:
            return inputArgs

        requiredArgs = ["-maxstackmemoryusageconsensus=0",
                        "-minminingtxfee=0.000005",
                        "-excessiveblocksize=0"]

        allSetArgs = []
        #Config file parameters should be checked as well
        configFilename = os.path.join(self.datadir, "prettywomancoin.conf")
        with open(configFilename, 'r', encoding='utf8') as configFile:
            configFileArg = configFile.readline()
            while configFileArg:
                allSetArgs += ["-" + configFileArg.rstrip()]
                configFileArg = configFile.readline()
        
        allSetArgs += inputArgs

        for currentArg in allSetArgs:
            checkCurentAt = currentArg.find("=")
            if checkCurentAt == -1:
                checkCurentAt = len(currentArg)
            for requiredArg in requiredArgs:
                checkRequiredAt = requiredArg.find("=")
                if checkRequiredAt == -1:
                    checkRequiredAt = len(requiredArg)
                if requiredArg[:checkRequiredAt] == currentArg[:checkCurentAt]:
                   requiredArgs.remove(requiredArg)
                   break

        return inputArgs + requiredArgs

    def start(self, runNodesWithRequiredParams, extra_args=None, stderr=None):
        """Start the node."""
        if os.path.isfile(os.path.join(self.datadir, "regtest", ".cookie")):
            # remove old .cookie file so that it is not accidentally used in wait_for_rpc_connection before new one is created by node during startup
            os.remove(os.path.join(self.datadir, "regtest", ".cookie"))
        if extra_args is None:
            extra_args = self.extra_args
        if stderr is None:
            stderr = self.stderr
        self.process = subprocess.Popen(self.setRequiredArgs(self.args + extra_args, runNodesWithRequiredParams), stderr=stderr)
        TestNode_process_list.append(self.process) # Add node process to list of running external processes
        self.running = True
        self.log.debug("prettywomancoind started, waiting for RPC to come up")

    def wait_for_rpc_connection(self):
        """Sets up an RPC connection to the prettywomancoind process. Returns False if unable to connect."""
        # Poll at a rate of four times per second
        poll_per_s = 4
        for _ in range(poll_per_s * self.rpc_timeout):
            assert self.process.poll(
            ) is None, "prettywomancoind exited with status %i during initialization" % self.process.returncode
            try:
                self.rpc = get_rpc_proxy(rpc_url(self.datadir, self.index, self.rpchost),
                                         self.index, timeout=self.rpc_timeout, coveragedir=self.coverage_dir)
                self.rpc.getblockcount()
                # If the call to getblockcount() succeeds then the RPC connection is up
                self.rpc_connected = True
                self.url = self.rpc.url
                self.log.debug("RPC successfully started")
                return
            except IOError as e:
                if e.errno != errno.ECONNREFUSED:  # Port not yet open?
                    raise  # unknown IO error
            except JSONRPCException as e:  # Initialization phase
                if e.error['code'] != -28:  # RPC in warmup?
                    raise  # unknown JSON RPC exception
            except ValueError as e:  # cookie file not found and no rpcuser or rpcassword. prettywomancoind still starting
                if "No RPC credentials" not in str(e):
                    raise
            time.sleep(1.0 / poll_per_s)
        raise AssertionError("Unable to connect to prettywomancoind")

    def get_wallet_rpc(self, wallet_name):
        assert self.rpc_connected
        assert self.rpc
        wallet_path = "wallet/%s" % wallet_name
        return self.rpc / wallet_path

    def stop_node(self):
        """Stop the node."""
        if not self.running:
            return
        self.log.debug("Stopping node")
        try:
            self.stop()
        except http.client.CannotSendRequest:
            self.log.exception("Unable to stop node.")

    def is_node_stopped(self, assert_zero_exit_code=True):
        """Checks whether the node has stopped.

        Returns True if the node has stopped. False otherwise.
        This method is responsible for freeing resources (self.process)."""
        if not self.running:
            return True
        return_code = self.process.poll()
        if return_code is None:
            return False

        TestNode_process_list.remove(self.process) # Remove node process from list of running external processes

        if assert_zero_exit_code:
            # process has stopped. Assert that it didn't return an error code.
            assert_equal(return_code, 0)
        self.running = False
        self.process = None
        self.rpc_connected = False
        self.rpc = None
        self.log.debug("Node stopped")
        return True

    def wait_until_stopped(self, timeout=BITCOIND_PROC_WAIT_TIMEOUT):
        wait_until(self.is_node_stopped, timeout=timeout)

    def wait_for_exit(self, timeout):
        self.process.wait(timeout)
        assert self.is_node_stopped(assert_zero_exit_code=False) # Check that process has quit and cleanup resources

    def node_encrypt_wallet(self, passphrase):
        """"Encrypts the wallet.

        This causes prettywomancoind to shutdown, so this method takes
        care of cleaning up resources."""
        self.encryptwallet(passphrase)
        self.wait_until_stopped()

    def relay_fee(self, cached=True):
        if not self.relay_fee_cache or not cached:
            self.relay_fee_cache = self.getnetworkinfo()["relayfee"]

        return self.relay_fee_cache

    def calculate_fee(self, tx):
        return int(self.relay_fee() * len(ToHex(tx)) * COIN)


class TestNodeCLI():
    """Interface to prettywomancoin-cli for an individual node"""

    def __init__(self, binary, datadir):
        self.args = []
        self.binary = binary
        self.datadir = datadir
        self.input = None

    def __call__(self, *args, input=None):
        # TestNodeCLI is callable with prettywomancoin-cli command-line args
        self.args = [str(arg) for arg in args]
        self.input = input
        return self

    def __getattr__(self, command):
        def dispatcher(*args, **kwargs):
            return self.send_cli(command, *args, **kwargs)
        return dispatcher

    def send_cli(self, command, *args, **kwargs):
        """Run prettywomancoin-cli command. Deserializes returned string as python object."""

        pos_args = [str(arg) for arg in args]
        named_args = [str(key) + "=" + str(value)
                      for (key, value) in kwargs.items()]
        assert not (
            pos_args and named_args), "Cannot use positional arguments and named arguments in the same prettywomancoin-cli call"

        p_args = [self.binary, "-datadir=" + self.datadir] + self.args
        if named_args:
            p_args += ["-named"]
        p_args += [command] + pos_args + named_args
        process = subprocess.Popen(p_args, stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
        cli_stdout, cli_stderr = process.communicate(input=self.input)
        returncode = process.poll()
        if returncode:
            # Ignore cli_stdout, raise with cli_stderr
            raise subprocess.CalledProcessError(
                returncode, self.binary, output=cli_stderr)
        return json.loads(cli_stdout, parse_float=decimal.Decimal)




def TestNode_kill_running_processes():
    """ Kill all started external processes that are still running in reverse order they were added to array """
    for p in reversed(TestNode_process_list):
        if(p.poll() is None):
            print("Killing sub-process " + str(p.pid))
            p.kill()
            p.wait(timeout=1)
    TestNode_process_list.clear()

atexit.register(TestNode_kill_running_processes)
