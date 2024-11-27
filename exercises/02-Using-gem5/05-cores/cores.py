from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.cachehierarchies.classic.private_l1_cache_hierarchy import PrivateL1CacheHierarchy
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.resources.resource import obtain_resource
from gem5.simulate.simulator import Simulator

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("cpu_type", type=str, help="The cpu type")

args = parser.parse_args()

from my_processor import BigProcessor, LittleProcessor
if args.cpu_type == "Big":
    processor = BigProcessor()
elif args.cpu_type == "Little":
    processor = LittleProcessor()
elif args.cpu_type == "Simple":
    processor = SimpleProcessor(cpu_type=CPUTypes.TIMING,
                            isa=ISA.RISCV,
                            num_cores=1)

board = SimpleBoard(
    clk_freq="3GHz",
    processor=processor,
    memory=SingleChannelDDR4_2400("1GiB"),
    cache_hierarchy=PrivateL1CacheHierarchy(
        l1d_size="32KiB", l1i_size="32KiB"
    ),
)

workload = obtain_resource("riscv-matrix-multiply-run")
board.set_workload(workload)
sim = Simulator(board=board)
sim.run()

stats = sim.get_simstats()

print(f"Total time: {stats.simTicks.value / stats.simFreq.value * 1e3:0.3f}ms")
print(f"Total instructions: {int(stats.simInsts.value)}")
