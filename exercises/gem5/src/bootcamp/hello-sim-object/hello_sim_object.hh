#ifndef __BOOTCAMP_HELLO_SIM_OBJECT_HELLO_SIM_OBJECT_HH__
#define __BOOTCAMP_HELLO_SIM_OBJECT_HELLO_SIM_OBJECT_HH__

#include "params/HelloSimObject.hh"
#include "sim/sim_object.hh"
#include "sim/eventq.hh"

namespace gem5
{

class HelloSimObject: public SimObject
{
  public:
    HelloSimObject(const HelloSimObjectParams& params);
    virtual void startup() override;

  private:
    int remainingHellosToPrintByEvent;
    EventFunctionWrapper nextHelloEvent;
    void processNextHelloEvent();
};

} // namespace gem5

#endif // __BOOTCAMP_HELLO_SIM_OBJECT_HELLO_SIM_OBJECT_HH__
