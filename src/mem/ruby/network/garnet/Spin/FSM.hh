#ifndef __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/network/garnet/Spin/CommonTypes.hh"
#include "mem/ruby/network/garnet/Spin/MoveManager.hh"
#include "mem/ruby/network/garnet/Spin/ProbeManager.hh"

namespace gem5 {

namespace ruby {

namespace garnet {
class Router;

namespace spin {

class SpinFSM : public Consumer
{
  public:
    SpinFSM(Router *router);

    void flitArrive(flit *flit, int inport, int vc, Tick time);
    void wakeup() override;
    void print(std::ostream &out) const override {}
    void schedule_wakeup(Cycles time) {
        // wake up after time cycles
        scheduleEvent(time);
    }
    void flitLeave(flit *flit, Tick time);
    void registerSpinMessage(SpinMessage *msg, int inport, Tick time);
    void processSpinMessage();

  protected:
    ProbeManager pm;
    MoveManager mm;
    Router *m_router;
    FSMState m_state;
    int vc;
    int inport;
    flit *flit_watch;
    Cycles counter;
    static Cycles detectionThreshold;
    Tick getTick(Cycles c);
    Cycles getCycles(Tick t);

    struct MSGStore
    {
        SpinMessage *msg;
        int inport;
        MSGStore() : msg(NULL), inport(-1) {}
    } msg_store;
    int sender_id;
    bool is_deadlock;
};
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
