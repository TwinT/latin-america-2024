from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.cachehierarchies.classic.private_l1_shared_l2_cache_hierarchy import PrivateL1SharedL2CacheHierarchy
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.resources.resource import obtain_resource
import m5
from gem5.simulate.simulator import (
    ExitEvent,
    Simulator,
)

import argparse

# parser = argparse.ArgumentParser()
# parser.add_argument("cpu_type", type=str, help="The cpu type")

# args = parser.parse_args()

cache_hierarchy = PrivateL1SharedL2CacheHierarchy(
    l1d_size="64kB", l1i_size="64kB", l2_size="1MB",
)

memory = SingleChannelDDR4_2400()
processor = SimpleProcessor(
    cpu_type=CPUTypes.TIMING,
    isa=ISA.RISCV,
    num_cores=1
)
board = SimpleBoard(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy,
)

board.set_se_binary_workload(obtain_resource("matrix-multiply"))

def workbegin_handler():
    print("Workbegin handler")
    m5.stats.dump()
    m5.stats.reset()
    yield False

def workend_handler():
    print("Workend handler")
    m5.stats.dump()
    m5.stats.reset()
    yield False

sim = Simulator(board=board,
    on_exit_event={
        ExitEvent.WORKBEGIN: workbegin_handler(),
        ExitEvent.WORKEND: workend_handler()
    }
)
sim.run()

stats = sim.get_simstats()

print(f"Total time: {stats.simTicks.value / stats.simFreq.value * 1e3:0.3f}ms")
print(f"Total instructions: {int(stats.simInsts.value)}")
