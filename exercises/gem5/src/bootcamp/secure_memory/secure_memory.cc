#include "bootcamp/secure_memory/secure_memory.hh"

#include "debug/SecureMemory.hh"
#include <algorithm>

namespace gem5
{

SecureMemory::SecureMemory(const SecureMemoryParams& params):
    ClockedObject(params),
    cpuSidePort(this, name() + ".cpu_side_port"),
    memSidePort(this, name() + ".mem_side_port"),
    bufferEntries(params.inspection_buffer_entries),
    buffer(clockPeriod()),
    responseBufferEntries(params.response_buffer_entries),
    responseBuffer(clockPeriod()),
    nextReqSendEvent([this](){ processNextReqSendEvent(); }, name() + ".nextReqSendEvent"),
    nextReqRetryEvent([this](){ processNextReqRetryEvent(); }, name() + ".nextReqRetryEvent"),
    nextRespSendEvent([this](){ processNextRespSendEvent(); }, name() + ".nextRespSendEvent"),
    nextRespRetryEvent([this](){ processNextRespRetryEvent(); }, name() + ".nextRespRetryEvent"),
    stats(this)
{}

    Port& SecureMemory::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "cpu_side_port") {
        return cpuSidePort;
    } else if (if_name == "mem_side_port") {
        return memSidePort;
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}

Tick SecureMemory::CPUSidePort::recvAtomic(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Received pkt: %s in atomic mode.\n", __func__, pkt->print());
    return owner->recvAtomic(pkt);
}

void SecureMemory::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Received pkt: %s in functional mode.\n", __func__, pkt->print());
    owner->recvFunctional(pkt);
}

AddrRangeList
SecureMemory::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool SecureMemory::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Received pkt: %s in timing mode.\n", __func__, pkt->print());
    if (owner->recvTimingReq(pkt)) {
        return true;
    }
    needToSendRetry = true;
    return false;
}

Tick
SecureMemory::align(Tick when)
{
    return clockEdge((Cycles) std::ceil((when - curTick()) / clockPeriod()));
}

AddrRangeList
SecureMemory::getAddrRanges() const
{
    return memSidePort.getAddrRanges();
}

void
SecureMemory::recvFunctional(PacketPtr pkt)
{
    memSidePort.sendFunctional(pkt);
}

Tick
SecureMemory::recvAtomic(PacketPtr pkt)
{
    return clockPeriod() + memSidePort.sendAtomic(pkt);
}

bool
SecureMemory::recvTimingReq(PacketPtr pkt)
{
    if (buffer.size() >= bufferEntries) {
        return false;
    }
    buffer.push(pkt, curTick());
    scheduleNextReqSendEvent(nextCycle());
    return true;
}

void
SecureMemory::MemSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blocked(), "Should never try to send if blocked!");

    DPRINTF(SecureMemory, "%s: Sending pkt: %s.\n", __func__, pkt->print());
    if (!sendTimingReq(pkt)) {
        DPRINTF(SecureMemory, "%s: Failed to send pkt: %s.\n", __func__, pkt->print());
        blockedPacket = pkt;
    }
}

void
SecureMemory::processNextReqRetryEvent()
{
    panic_if(!cpuSidePort.needRetry(), "Should never try to send retry if not needed!");
    cpuSidePort.sendRetryReq();
}

void
SecureMemory::scheduleNextReqRetryEvent(Tick when)
{
    if (cpuSidePort.needRetry() && !nextReqRetryEvent.scheduled()) {
        schedule(nextReqRetryEvent, align(when));
    }
}

void
SecureMemory::processNextReqSendEvent()
{
    panic_if(memSidePort.blocked(), "Should never try to send if blocked!");
    panic_if(!buffer.hasReady(curTick()), "Should never try to send if no ready packets!");

    stats.numRequestsFwded++;
    stats.totalbufferLatency += curTick() - buffer.frontTime();

    PacketPtr pkt = buffer.front();
    memSidePort.sendPacket(pkt);
    buffer.pop();

    scheduleNextReqRetryEvent(nextCycle());
    scheduleNextReqSendEvent(nextCycle());

}

void
SecureMemory::scheduleNextReqSendEvent(Tick when)
{
    bool port_avail = !memSidePort.blocked();
    bool have_items = !buffer.empty();

    if (port_avail && have_items && !nextReqSendEvent.scheduled()) {
        Tick schedule_time = align(buffer.firstReadyTime());
        schedule(nextReqSendEvent, schedule_time);
    }
}

void
SecureMemory::MemSidePort::recvReqRetry()
{
    panic_if(!blocked(), "Should never receive retry if not blocked!");

    DPRINTF(SecureMemory, "%s: Received retry signal.\n", __func__);
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);

    if (!blocked()) {
        owner->recvReqRetry();
    }
}

void
SecureMemory::recvReqRetry()
{
    scheduleNextReqSendEvent(nextCycle());
}

bool SecureMemory::MemSidePort::recvTimingResp(PacketPtr pkt){
    DPRINTF(SecureMemory, "%s: Received pkt: %s in timing mode.\n", __func__, pkt->print());
    if (owner->recvTimingResp(pkt)) {
        return true;
    }
    needToSendRetry = true;
    return false;
}

void SecureMemory::CPUSidePort::sendPacket(PacketPtr pkt){
        panic_if(blocked(), "Should never try to send if blocked!");

    DPRINTF(SecureMemory, "%s: Sending pkt: %s.\n", __func__, pkt->print());
    if (!sendTimingResp(pkt)) {
        DPRINTF(SecureMemory, "%s: Failed to send pkt: %s.\n", __func__, pkt->print());
        blockedPacket = pkt;
    }
}

void SecureMemory::CPUSidePort::recvRespRetry(){
    panic_if(!blocked(), "Should never receive retry if not blocked!");

    DPRINTF(SecureMemory, "%s: Received retry signal.\n", __func__);
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);

    if (!blocked()) {
        owner->recvRespRetry();
    }
}

bool SecureMemory::recvTimingResp(PacketPtr pkt){
    if (responseBuffer.size() >= responseBufferEntries) {
        return false;
    }
    responseBuffer.push(pkt, curTick());
    scheduleNextRespSendEvent(nextCycle());
    return true;
}

void SecureMemory::recvRespRetry(){
    scheduleNextRespSendEvent(nextCycle());
}

void SecureMemory::processNextRespSendEvent(){
    panic_if(cpuSidePort.blocked(), "Should never try to send if blocked!");
    panic_if(!responseBuffer.hasReady(curTick()), "Should never try to send if no ready packets!");

    stats.totalResponseBufferLatency += curTick() - responseBuffer.frontTime();
    stats.numResponsesFwded++;
    
    PacketPtr pkt = responseBuffer.front();
    cpuSidePort.sendPacket(pkt);
    responseBuffer.pop();

    scheduleNextRespRetryEvent(nextCycle());
    scheduleNextRespSendEvent(nextCycle());

}

void SecureMemory::scheduleNextRespSendEvent(Tick when){
    bool port_avail = !cpuSidePort.blocked();
    bool have_items = !responseBuffer.empty();

    if (port_avail && have_items && !nextRespSendEvent.scheduled()) {
        Tick schedule_time = align(std::max(when, responseBuffer.firstReadyTime()));
        schedule(nextRespSendEvent, schedule_time);
    }
}
void SecureMemory::processNextRespRetryEvent(){
    panic_if(!memSidePort.needRetry(), "Should never try to send retry if not needed!");
    memSidePort.sendRetryResp();
}

void
SecureMemory::scheduleNextRespRetryEvent(Tick when)
{
    if (memSidePort.needRetry() && !nextRespRetryEvent.scheduled()) {
        schedule(nextRespRetryEvent, align(when));
    }
}

void SecureMemory::init()
{
    cpuSidePort.sendRangeChange();
}

SecureMemory::SecureMemoryStats::SecureMemoryStats(SecureMemory* secure_memory):
    statistics::Group(secure_memory),
    ADD_STAT(totalbufferLatency, statistics::units::Tick::get(), "Total inspection buffer latency."),
    ADD_STAT(numRequestsFwded, statistics::units::Count::get(), "Number of requests forwarded."),
    ADD_STAT(totalResponseBufferLatency, statistics::units::Tick::get(), "Total response buffer latency."),
    ADD_STAT(numResponsesFwded, statistics::units::Count::get(), "Number of responses forwarded.")
{}

uint64_t
SecureMemory::getHmacAddr(uint64_t child_addr)
{
    AddrRangeList ranges = memSidePort.getAddrRanges();
    assert(ranges.size() == 1);

    uint64_t start = ranges.front().start();
    uint64_t end = ranges.front().end();

    if (!(child_addr >= start && child_addr < end)) {
        // this is a check for something that isn't metadata
        return (uint64_t) -1;
    }

    // raw location, not word aligned
    uint64_t hmac_addr = integrity_levels[hmac_level] + ((child_addr / BLOCK_SIZE) * HMAC_SIZE);

    // word aligned
    return hmac_addr - (hmac_addr % BLOCK_SIZE);
}

uint64_t
SecureMemory::getParentAddr(uint64_t child_addr)
{
    AddrRangeList ranges = memSidePort.getAddrRanges();
    assert(ranges.size() == 1);

    uint64_t start = ranges.front().start();
    uint64_t end = ranges.front().end();

    if (child_addr >= start && child_addr < end) {
        // child is data, get the counter
        return integrity_levels[counter_level] + ((child_addr / PAGE_SIZE) * BLOCK_SIZE);
    }

    for (int i = counter_level; i > root_level; i--) {
        if (child_addr >= integrity_levels[i] && child_addr < integrity_levels[i - 1]) {
            // we belong to this level
            uint64_t index_in_level = (child_addr - integrity_levels[i]) / BLOCK_SIZE;
            return integrity_levels[i - 1] + ((index_in_level / ARITY) * BLOCK_SIZE);
        }
    }

    assert(child_addr == integrity_levels[root_level]);
    // assert(false); // we shouldn't ever get here
    return (uint64_t) -1;
}

}
