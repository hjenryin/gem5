#ifndef __MEM_RUBY_NETWORK_GARNET_0_SPIN_PROBEMANAGER_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_SPIN_PROBEMANAGER_HH__
#include "mem/ruby/network/garnet/Spin/SpinMessage.hh"

namespace gem5 {

namespace ruby {

namespace garnet {

namespace spin {
class SpinFSM;
class ProbeManager
{
  private:
    SpinFSM *fsm;

  public:
    ProbeManager(SpinFSM *fsm);
    void sendNewProbe(int port, int watch_inport);
    bool handleProbe(SpinMessage *message, int inport, bool from_self);

  private:
    void sendMove(LoopBuffer path);

    int get_router_id();
    bool test_outport(int inport, int invc, int outport);
};
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
#endif // __MEM_RUBY_NETWORK_GARNET_0_FSM_PROBEMANAGER_HH__
