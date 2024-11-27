"""
This script creates a simple system with a traffic generator to test memory

$ gem5 memory-test.py
"""

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("rate", type=str, help="The rate of the generator")
parser.add_argument("rd_perc", type=int, help="The percentage of read requests")
parser.add_argument("memory", type=str, help="The percentage of read requests")
parser.add_argument("max_addr", type=int, help="The percentage of read requests")

args = parser.parse_args()

from gem5.components.boards.test_board import TestBoard
from gem5.components.cachehierarchies.classic.no_cache import NoCache
from gem5.components.memory.simple import SingleChannelSimpleMemory
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.memorself.l3_cache = L3Cache(size=self._l3_size, assoc=self._l3_assoc)

# Connect the L3 cache to the system crossbar and L3 crossbar
self.l3_cache.mem_side = self.membus.cpu_side_ports
self.l3_cache.cpu_side = self.l3_bus.mem_side_portsy.multi_channel import ChanneledMemory, DualChannelDDR4_2400
from gem5.components.memory.dram_interfaces.lpddr5 import LPDDR5_6400_1x16_BG_BL32
from gem5.components.processors.linear_generator import LinearGenerator
from gem5.components.processors.random_generator import RandomGenerator
from gem5.simulate.simulator import Simulator
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import PrivateL1PrivateL2CacheHierarchy

def get_memory(mem_type: str):
    if mem_type == "simple":
        return SingleChannelSimpleMemory(
            latency="20ns", bandwidth="32GiB/s", latency_var="0s", size="1GiB"
        )
    elif mem_type == "DDR4":
        return DualChannelDDR4_2400(size="1GiB")
    elif mem_type == "SC_LPDDR5":
        return ChanneledMemory(LPDDR5_6400_1x16_BG_BL32, 4, 64)

board = TestBoard(
    clk_freq="3GHz",
    # generator=LinearGenerator(num_cores=1, rate=args.rate, rd_perc=args.rd_perc),
    generator=RandomGenerator(num_cores=1, max_addr=args.max_addr, rate=args.rate, rd_perc=args.rd_perc, duration="1ms"),
    memory=get_memory(args.memory),
    cache_hierarchy=PrivateL1PrivateL2CacheHierarchy(l1d_size="32KiB",
        l1i_size="32KiB",
        l2_size="256KiB",
        l3_size="1MiB"),
)

simulator = Simulator(board)
simulator.run()

stats = simulator.get_simstats()
seconds = stats.simTicks.value / stats.simFreq.value
total_bytes = (
    stats.board.processor.cores[0].generator.bytesRead.value
    + stats.board.processor.cores[0].generator.bytesWritten.value
)
latency = (
    stats.board.processor.cores[0].generator.totalReadLatency.value
    / stats.board.processor.cores[0].generator.totalReads.value
)
print(f"Total bandwidth: {total_bytes / seconds / 2**30:0.2f} GiB/s")
print(f"Average latency: {latency / stats.simFreq.value * 1e9:0.2f} ns")

#
#
