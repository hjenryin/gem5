#ifndef __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
#include "mem/ruby/network/garnet/FSM/CommonTypes.hh"

namespace gem5 {

namespace ruby {

namespace garnet {
class Router;

namespace fsm {

class SpinFSM : public Consumer
{
  public:
    SpinFSM(Router *router);
    void flitArrive(flit *flit, int inport, int vc, Tick time);
    void wakeup() override;
    void schedule_wakeup(Cycles time) {
        // wake up after time cycles
        scheduleEvent(time);
    }
    void flitLeave(flit *flit, Tick time);

  protected:
    Router *m_router;
    FSMState m_state;
    int vc;
    int inport;
    flit *flit;
    Tick time;
    static Cycles detectionThreshold;
    Tick getTick(Cycles c) { return m_router->cyclesToTicks(c); }
};
} // namespace fsm
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
