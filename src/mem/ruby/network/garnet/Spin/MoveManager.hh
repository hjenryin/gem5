#ifndef __MEM_RUBY_NETWORK_GARNET_0_FSM_MOVE_MANAGER_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_FSM_MOVE_MANAGER_HH__
#include "mem/ruby/network/garnet/Spin/CommonTypes.hh"
#include "mem/ruby/network/garnet/Spin/SpinMessage.hh"

namespace gem5 {
namespace ruby {
namespace garnet {
namespace spin {
class MoveManager
{
  public:
    void handleMessage(SpinMessage *msg, int inport);
};
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
#endif // __MEM_RUBY_NETWORK_GARNET_0_FSM_MOVE_MANAGER_HH__
