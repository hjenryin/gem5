#ifndef __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
#include <set>

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

class SpinFSM
{
  public:
    SpinFSM(Router *router, bool spin_enabled);

    void flitArrive(flit *flit, int inport, int vc);
    void wakeup();

    void flitLeave(flit *flit);
    void registerSpinMessage(SpinMessage *msg, int inport, Tick time);
    void processSpinMessage();
    FSMState get_state() { return m_state; }
    Router *get_router() { return m_router; }
    void sendMessage(SpinMessage *msg, int outport);
    void setLoopBuf(LoopBuffer buf) { loopBuf = buf; }
    LoopBuffer getLoopBuf() { return loopBuf; }
    bool get_spinning() { return spinning; }
    int get_counter_inport() { return inport; }
    int set_current_sender_ID(int id) { return sender_id = id; }
    int get_current_sender_ID() { return sender_id; }
    static int getCurrentPriority(int sender_id);
    bool test_outport_used(int outport){
      return msg_sent_this_cycle.find(outport) != msg_sent_this_cycle.end();
    }

  protected:
    bool spin_enabled;
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
    flit *roundRobinInVC(int &inport, int &invc);
    void re_init();
    void pushSpin();

    struct MSGStore
    {
        SpinMessage *msg;
        int inport;

        MSGStore() : msg(NULL), inport(-1) {}
        void clear() {
            msg = NULL;
            inport = -1;
        }
    } msg_store;

    int sender_id;
    void reset_is_deadlock();
    bool is_deadlock;
    Cycles curCycles() { return getCycles(curTick()); }
    bool spinning = false;
    Cycles latest_cycle_debug;
    std::set<int> msg_sent_this_cycle;
};
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_FSM_FSM_HH__
