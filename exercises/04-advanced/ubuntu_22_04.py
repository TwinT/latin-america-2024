from gem5.prebuilt.demo.x86_demo_board import X86Board
from gem5.resources.resource import obtain_resource
from gem5.simulate.simulator import Simulator
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.memory.simple import SingleChannelSimpleMemory
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.simulate.exit_event import ExitEvent
from gem5.components.memory.multi_channel import DualChannelDDR4_2400
from gem5.components.cachehierarchies.classic.private_l1_cache_hierarchy import PrivateL1CacheHierarchy

processor = SimpleProcessor(cpu_type=CPUTypes.KVM, isa=ISA.X86, num_cores=8)
board = X86Board(processor=processor, clk_freq="3GHz", memory=DualChannelDDR4_2400(size="3GiB"),
        cache_hierarchy=PrivateL1CacheHierarchy(
            l1d_size="32KiB",
            l1i_size="32KiB"
        ))

board.set_workload(obtain_resource("x86-ubuntu-22.04-boot-with-systemd"))
board.append_kernel_arg("interactive=true")


for proc in processor.cores:
    proc.core.usePerf = False

def exit_event_handler():
    print("first exit event: Kernel booted")
    yield False
    print("second exit event: In after boot")
    yield False
    print("third exit event: After run script")
    yield True

simulator = Simulator(
    board=board,
    on_exit_event={
        ExitEvent.EXIT: exit_event_handler(),
    },
)
simulator.run()
