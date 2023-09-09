#ifndef __MEM_RUBY_NETWORK_GARNET_0_SPIN_PROBEMANAGER_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_SPIN_PROBEMANAGER_HH__
#include "mem/ruby/network/garnet/Spin/SpinMessage.hh"

namespace gem5 {

namespace ruby {

namespace garnet {

namespace spin {
class ProbeManager
{
  private:
    SpinFSM *fsm;

  public:
    ProbeManager(SpinFSM *fsm);
    void sendNewProbe(int port);
    void handleMessage(SpinMessage *message, int port, bool from_self);

  private:
    int get_sender_id() { return fsm->get_router()->get_id(); }
    bool test_outport(int inport, int invc, int outport);
    bool test_inport(int inport);
};
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
#endif // __MEM_RUBY_NETWORK_GARNET_0_FSM_PROBEMANAGER_HH__
