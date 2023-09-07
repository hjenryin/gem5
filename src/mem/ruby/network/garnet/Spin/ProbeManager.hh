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
    Router *router;

  public:
    ProbeManager(Router *router);
    void send(SpinMessage *message, int port);
    void forward(SpinMessage *message, int port);
};
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
#endif // __MEM_RUBY_NETWORK_GARNET_0_FSM_PROBEMANAGER_HH__
