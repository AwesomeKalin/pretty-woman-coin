#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Prettywomancoin Core developers
# Copyright (c) 2019 Prettywomancoin Association
# Distributed under the Open PWC software license, see the accompanying file LICENSE.
"""Compare two or more prettywomancoinds to each other.

To use, create a class that implements get_tests(), and pass it in
as the test generator to TestManager.  get_tests() should be a python
generator that returns TestInstance objects.  See below for definition.

TestNode behaves as follows:
    Configure with a BlockStore and TxStore
    on_inv: log the message but don't request
    on_headers: log the chain tip
    on_pong: update ping response map (for synchronization)
    on_getheaders: provide headers via BlockStore
    on_getdata: provide blocks via BlockStore
"""

from .mininode import *
from .blockstore import BlockStore, TxStore
from .util import p2p_port, wait_until

import logging

logger = logging.getLogger("TestFramework.comptool")

global mininode_lock


class RejectResult():
    """Outcome that expects rejection of a transaction or block."""

    def __init__(self, code, reason=b''):
        self.code = code
        self.reason = reason

    def match(self, other):
        if self.code != other.code:
            return False
        return other.reason.startswith(self.reason)

    def __repr__(self):
        return '%i:%s' % (self.code, self.reason or '*')

class DiscardResult():
    """Outcome that expects the silent discarding of a transaction."""
    pass

class TestNode(NodeConnCB):

    def __init__(self, block_store, tx_store):
        super().__init__()
        self.conn = None
        self.bestblockhash = None
        self.block_store = block_store
        self.block_request_map = {}
        self.tx_store = tx_store
        self.tx_request_map = {}
        self.block_reject_map = {}
        self.tx_reject_map = {}

        # When the pingmap is non-empty we're waiting for
        # a response
        self.pingMap = {}
        self.lastInv = []
        self.closed = False

    def on_close(self, conn):
        self.closed = True

    def add_connection(self, conn):
        self.conn = conn

    def on_headers(self, conn, message):
        if len(message.headers) > 0:
            best_header = message.headers[-1]
            best_header.calc_sha256()
            self.bestblockhash = best_header.sha256

    def on_getheaders(self, conn, message):
        response = self.block_store.headers_for(
            message.locator, message.hashstop)
        if response is not None:
            conn.send_message(response)

    def on_getdata(self, conn, message):
        [conn.send_message(r)
         for r in self.block_store.get_blocks(message.inv)]
        [conn.send_message(r)
         for r in self.tx_store.get_transactions(message.inv)]

        for i in message.inv:
            if i.type == 1:
                self.tx_request_map[i.hash] = True
            elif i.type == 2:
                self.block_request_map[i.hash] = True

    def on_inv(self, conn, message):
        self.lastInv = [x.hash for x in message.inv]

    def on_pong(self, conn, message):
        try:
            del self.pingMap[message.nonce]
        except KeyError:
            raise AssertionError(
                "Got pong for unknown ping [%s]" % repr(message))

    def on_reject(self, conn, message):
        if message.message == b'tx':
            self.tx_reject_map[message.data] = RejectResult(
                message.code, message.reason)
        if message.message == b'block':
            self.block_reject_map[message.data] = RejectResult(
                message.code, message.reason)

    def send_inv(self, obj):
        mtype = CInv.BLOCK if isinstance(obj, CBlock) else CInv.TX
        self.conn.send_message(msg_inv([CInv(mtype, obj.sha256)]))

    def send_getheaders(self):
        # We ask for headers from their last tip.
        m = msg_getheaders()
        m.locator = self.block_store.get_locator(self.bestblockhash)
        self.conn.send_message(m)

    def send_header(self, header):
        m = msg_headers()
        m.headers.append(header)
        self.conn.send_message(m)

    # This assumes BIP31
    def send_ping(self, nonce):
        self.pingMap[nonce] = True
        self.conn.send_message(msg_ping(nonce))

    def received_ping_response(self, nonce):
        return nonce not in self.pingMap

    def send_mempool(self):
        self.lastInv = []
        self.conn.send_message(msg_mempool())

# TestInstance:
#
# Instances of these are generated by the test generator, and fed into the
# comptool.
#
# "blocks_and_transactions" should be an array of
#    [obj, True/False/None, hash/None]:
#  - obj is either a CBlock, CBlockHeader, or a CTransaction, and
#  - the second value indicates whether the object should be accepted
#    into the blockchain or mempool (for tests where we expect a certain
#    answer), or "None" if we don't expect a certain answer and are just
#    comparing the behavior of the nodes being tested.
#  - the third value is the hash to test the tip against (if None or omitted,
#    use the hash of the block)
#  - NOTE: if a block header, no test is performed; instead the header is
#    just added to the block_store.  This is to facilitate block delivery
#    when communicating with headers-first clients (when withholding an
#    intermediate block).
# sync_every_block: if True, then each block will be inv'ed, synced, and
#    nodes will be tested based on the outcome for the block.  If False,
#    then inv's accumulate until all blocks are processed (or max inv size
#    is reached) and then sent out in one inv message.  Then the final block
#    will be synced across all connections, and the outcome of the final
#    block will be tested.
# sync_every_tx: analogous to behavior for sync_every_block, except if outcome
#    on the final tx is None, then contents of entire mempool are compared
#    across all connections.  (If outcome of final tx is specified as true
#    or false, then only the last tx is tested against outcome.)


class TestInstance():
    def __init__(self, objects=None, sync_every_block=True, sync_every_tx=False, sync_timeout=300, timeout_to_requested_block=None):
        self.blocks_and_transactions = objects if objects else []
        self.sync_every_block = sync_every_block
        self.sync_every_tx = sync_every_tx
        self.sync_timeout = sync_timeout
        self.timeout_to_requested_block = timeout_to_requested_block


class TestManager():

    def __init__(self, testgen, datadir):
        self.test_generator = testgen
        self.connections = []
        self.test_nodes = []
        self.block_store = BlockStore(datadir)
        self.tx_store = TxStore(datadir)
        self.ping_counter = 1
        self.destAddr = '127.0.0.1'
        self.waitForPingTimeout = 60

    def add_all_connections(self, nodes):
        for i in range(len(nodes)):
            # Create a p2p connection to each node
            test_node = TestNode(self.block_store, self.tx_store)
            self.test_nodes.append(test_node)
            self.connections.append(
                NodeConn(self.destAddr, p2p_port(i), nodes[i], test_node))
            # Make sure the TestNode (callback class) has a reference to its
            # associated NodeConn
            test_node.add_connection(self.connections[-1])

    def clear_all_connections(self):
        self.connections = []
        self.test_nodes = []

    def wait_for_disconnections(self):
        def disconnected():
            return all(node.closed for node in self.test_nodes)
        wait_until(disconnected, timeout=10, lock=mininode_lock)

    def wait_for_verack(self, timeout=60):
        return all(node.wait_for_verack(timeout) for node in self.test_nodes)

    def wait_for_pings(self, counter, timeout=60):
        def received_pongs():
            if all(node.received_ping_response(counter) for node in self.test_nodes):
                # after we receive pong we need to check that there are no async
                # block/transaction processes still running
                for c in self.connections:
                    res=c.rpc.getblockchainactivity()
                    if sum(res.values())>0:
                        # this node is still processing some block/transaction
                        return False
                return True
            return False
        wait_until(received_pongs, lock=mininode_lock, timeout=timeout)

    # sync_blocks: Wait for all connections to request the blockhash given
    # then send get_headers to find out the tip of each node, and synchronize
    # the response by using a ping (and waiting for pong with same nonce).
    def sync_blocks(self, blockhash, num_blocks, timeout=60, timeout_to_requested_block=None):
        def blocks_requested():
            return all(
                blockhash in node.block_request_map and node.block_request_map[blockhash]
                for node in self.test_nodes
            )

        # --> error if not requested
        # Automatic (default) timeout is calculated as described below. In special cases,
        # manual override is possible with parameter 'timeout_to_requested_block'.
        # Measured values for processing blocks range from 0.008 to 0.035 s/block (debug build)
        # Processing gets slower with the amount of blocks (0.008 s/block @ 200 blocks, 0.035 s/block @ 1000 blocks)
        # We use a slightly higher value of 0.05s + an extra 30s for good measure.
        if timeout_to_requested_block is None:
            timeout_to_requested_block = 0.05*num_blocks+30

        wait_until(blocks_requested, timeout=timeout_to_requested_block, lock=mininode_lock)

        # Wait for all the blocks to finish processing
        [c.cb.send_ping(self.ping_counter) for c in self.connections]
        self.wait_for_pings(self.ping_counter, timeout=timeout)
        self.ping_counter += 1

        # Send getheaders message
        [c.cb.send_getheaders() for c in self.connections]

        # Send ping and wait for response -- synchronization hack
        [c.cb.send_ping(self.ping_counter) for c in self.connections]
        self.wait_for_pings(self.ping_counter, timeout=timeout)
        self.ping_counter += 1

    # Analogous to sync_block (see above)
    def sync_transaction(self, txhash, num_events):
        # Wait for nodes to request transaction
        def transaction_requested():
            return all(
                txhash in node.tx_request_map and node.tx_request_map[txhash]
                for node in self.test_nodes
            )

        # --> error if not requested
        # Observed data shows that during testing some responses take up to 2 seconds.
        # Timeout of 3s plus an extra 30s for good measure
        wait_until(transaction_requested, timeout=3*num_events+30, lock=mininode_lock)

        # We must wait for node to finish processing transactions before 'mempool' p2p message is sent
        [c.cb.send_ping(self.ping_counter) for c in self.connections]
        self.wait_for_pings(self.ping_counter)
        self.ping_counter += 1

        # Get the mempool
        [c.cb.send_mempool() for c in self.connections]

        # Send ping and wait for response -- synchronization hack
        [c.cb.send_ping(self.ping_counter) for c in self.connections]
        self.wait_for_pings(self.ping_counter)
        self.ping_counter += 1

        # Sort inv responses from each node
        with mininode_lock:
            [c.cb.lastInv.sort() for c in self.connections]

    def __check_results_outcome_none(self):
        with mininode_lock:
            for c in self.connections:
                if c.cb.bestblockhash != self.connections[0].cb.bestblockhash:
                    return False
        return True

    def __check_results_outcome_RejectResult(self, blockhash, outcome):
        # Check that block was rejected w/ code
        with mininode_lock:
            for c in self.connections:
                if c.cb.bestblockhash == blockhash:
                    return ('Block was not rejected: %064x' % (blockhash), False)

                if not blockhash in c.cb.block_reject_map:
                    return ('Block not in reject map: %064x' % (blockhash), True)

                if not outcome.match(c.cb.block_reject_map[blockhash]):
                    return ('Block rejected with %s instead of expected %s: %064x' % (
                        c.cb.block_reject_map[blockhash], outcome, blockhash), False)
        return ('', False)

    def __check_results_else(self, blockhash, outcome):
        with mininode_lock:
            for c in self.connections:
                if ((c.cb.bestblockhash == blockhash) != outcome):
                    return False
        return True

    # Verify that the tip of each connection all agree with each other, and
    # with the expected outcome (if given)
    def check_results(self, blockhash, outcome):
        if outcome is None:
            return self.__check_results_outcome_none()
        elif isinstance(outcome, RejectResult):
            error = ''
            for counter in range(0, 10):
                (error, retry) = self.__check_results_outcome_RejectResult(blockhash, outcome)

                if not error:
                    return True
                elif not retry:
                    break

                # sleep for a while as the rejection message might have
                # not been received yet due to the asynchronous nature
                # of that message
                time.sleep(0.5)

            logger.error(error)
            return False

        return self.__check_results_else(blockhash, outcome)

    # Either check that the mempools all agree with each other, or that
    # txhash's presence in the mempool matches the outcome specified.
    # This is somewhat of a strange comparison, in that we're either comparing
    # a particular tx to an outcome, or the entire mempools altogether;
    # perhaps it would be useful to add the ability to check explicitly that
    # a particular tx's existence in the mempool is the same across all nodes.
    def check_mempool(self, txhash, outcome):
        with mininode_lock:
            for c in self.connections:
                if outcome is None:
                    # Make sure the mempools agree with each other
                    if c.cb.lastInv != self.connections[0].cb.lastInv:
                        return False
                # Check that tx was rejected w/ code
                elif isinstance(outcome, RejectResult):
                    if txhash in c.cb.lastInv:
                        return False
                    if txhash not in c.cb.tx_reject_map:
                        logger.error('Tx not in reject map: %064x' % (txhash))
                        return False
                    if not outcome.match(c.cb.tx_reject_map[txhash]):
                        logger.error('Tx rejected with %s instead of expected %s: %064x' % (
                            c.cb.tx_reject_map[txhash], outcome, txhash))
                        return False
                elif isinstance(outcome, DiscardResult):
                    if txhash in c.cb.tx_reject_map:
                        logger.error('Tx in reject map: %064x' % (txhash))
                        return False
                    return not txhash in c.cb.lastInv
                elif ((txhash in c.cb.lastInv) != outcome):
                    return False
            return True

    def run(self):
        # Wait until verack is received
        self.wait_for_verack()

        test_number = 1
        for test_instance in self.test_generator.get_tests():
            # We use these variables to keep track of the last block
            # and last transaction in the tests, which are used
            # if we're not syncing on every block or every tx.
            [block, block_outcome, tip] = [None, None, None]
            [tx, tx_outcome] = [None, None]
            invqueue = []

            for test_obj in test_instance.blocks_and_transactions:
                b_or_t = test_obj[0]
                outcome = test_obj[1]
                # Determine if we're dealing with a block or tx
                if isinstance(b_or_t, CBlock):  # Block test runner
                    block = b_or_t
                    block_outcome = outcome
                    tip = block.sha256
                    # each test_obj can have an optional third argument
                    # to specify the tip we should compare with
                    # (default is to use the block being tested)
                    if len(test_obj) >= 3:
                        tip = test_obj[2]

                    # Add to shared block_store, set as current block
                    # If there was an open getdata request for the block
                    # previously, and we didn't have an entry in the
                    # block_store, then immediately deliver, because the
                    # node wouldn't send another getdata request while
                    # the earlier one is outstanding.
                    first_block_with_hash = True
                    if self.block_store.get(block.sha256) is not None:
                        first_block_with_hash = False
                    with mininode_lock:
                        self.block_store.add_block(block)
                        for c in self.connections:
                            if first_block_with_hash and block.sha256 in c.cb.block_request_map and c.cb.block_request_map[block.sha256] == True:
                                # There was a previous request for this block hash
                                # Most likely, we delivered a header for this block
                                # but never had the block to respond to the getdata
                                c.send_message(msg_block(block))
                            else:
                                c.cb.block_request_map[block.sha256] = False
                    # Either send inv's to each node and sync, or add
                    # to invqueue for later inv'ing.
                    if (test_instance.sync_every_block):
                        # if we expect success, send inv and sync every block
                        # if we expect failure, just push the block and see what happens.
                        if outcome == True:
                            [c.cb.send_inv(block) for c in self.connections]
                            self.sync_blocks(block.sha256, 1, timeout=test_instance.sync_timeout, timeout_to_requested_block=test_instance.timeout_to_requested_block)
                        else:
                            [c.send_message(msg_block(block))
                             for c in self.connections]
                            [c.cb.send_ping(self.ping_counter)
                             for c in self.connections]
                            self.wait_for_pings(self.ping_counter, self.waitForPingTimeout)
                            self.ping_counter += 1
                        if (not self.check_results(tip, outcome)):
                            raise AssertionError(
                                "Test failed at test %d" % test_number)
                    else:
                        invqueue.append(CInv(CInv.BLOCK, block.sha256))
                elif isinstance(b_or_t, CBlockHeader):
                    block_header = b_or_t
                    self.block_store.add_header(block_header)
                    [c.cb.send_header(block_header) for c in self.connections]

                else:  # Tx test runner
                    assert(isinstance(b_or_t, CTransaction))
                    tx = b_or_t
                    tx_outcome = outcome
                    # Add to shared tx store and clear map entry
                    with mininode_lock:
                        self.tx_store.add_transaction(tx)
                        for c in self.connections:
                            c.cb.tx_request_map[tx.sha256] = False
                    # Again, either inv to all nodes or save for later
                    if (test_instance.sync_every_tx):
                        [c.cb.send_inv(tx) for c in self.connections]
                        self.sync_transaction(tx.sha256, 1)
                        if (not self.check_mempool(tx.sha256, outcome)):
                            raise AssertionError(
                                "Test failed at test %d" % test_number)
                    else:
                        invqueue.append(CInv(CInv.TX, tx.sha256))
                # Ensure we're not overflowing the inv queue
                if len(invqueue) == c.maxInvElements:
                    [c.send_message(msg_inv(invqueue))
                     for c in self.connections]
                    invqueue = []

            # Do final sync if we weren't syncing on every block or every tx.
            if (not test_instance.sync_every_block and block is not None):
                if len(invqueue) > 0:
                    [c.send_message(msg_inv(invqueue))
                     for c in self.connections]
                    invqueue = []
                self.sync_blocks(block.sha256, len(test_instance.blocks_and_transactions), timeout=test_instance.sync_timeout, timeout_to_requested_block=test_instance.timeout_to_requested_block)
                if (not self.check_results(tip, block_outcome)):
                    raise AssertionError(
                        "Block test failed at test %d" % test_number)
            if (not test_instance.sync_every_tx and tx is not None):
                if len(invqueue) > 0:
                    [c.send_message(msg_inv(invqueue))
                     for c in self.connections]
                    invqueue = []
                self.sync_transaction(tx.sha256, len(
                    test_instance.blocks_and_transactions))
                if (not self.check_mempool(tx.sha256, tx_outcome)):
                    raise AssertionError(
                        "Mempool test failed at test %d" % test_number)

            logger.info("Test %d: PASS" % test_number)
            test_number += 1

        [c.disconnect_node() for c in self.connections]
        self.wait_for_disconnections()
        self.block_store.close()
        self.tx_store.close()
