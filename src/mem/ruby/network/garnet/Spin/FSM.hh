#ifndef __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/network/garnet/Spin/CommonTypes.hh"
#include "mem/ruby/network/garnet/Spin/LoopBuffer.hh"
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

    void flitArrive(flit *flit, int inport, int vc);
    void wakeup() override;
    void print(std::ostream &out) const override {}
    void schedule_wakeup(Cycles time) {
        // wake up after time cycles
        scheduleEvent(time);
    }
    void flitLeave(flit *flit);
    void registerSpinMessage(SpinMessage *msg, int inport, Tick time);
    void processSpinMessage();
    FSMState get_state() { return m_state; }
    Router *const get_router() { return m_router; }
    void sendMessage(SpinMessage *msg, int outport);
    void setLoopBuf(LoopBuffer buf) { loopBuf = buf; }

  protected:
    ProbeManager probeMan;
    MoveManager moveMan;
    LoopBuffer loopBuf;
    Router *m_router;
    FSMState m_state;
    int invc;
    int inport;
    flit *flit_watch;
    Cycles counter_start_time;
    static Cycles detectionThreshold;
    Tick getTick(Cycles c);
    Cycles getCycles(Tick t);
    Cycles currentTimeoutLimit;
    void checkTimeout();
    void reset_counter(Cycles c);
    void init();
    void re_init();
    void stop_counter();
    void spin_complete(); // after spin complete, transition to DL_DETECT or
                          // OFF depending on active VC

    struct MSGStore
    {
        SpinMessage *msg;
        int inport;
        MSGStore() : msg(NULL), inport(-1) {}
    } msg_store;
    int sender_id;
    bool is_deadlock;
    Cycles curCycles() { return getCycles(curTick()); }
};
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
