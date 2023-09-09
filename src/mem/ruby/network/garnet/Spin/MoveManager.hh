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
    MoveManager(SpinFSM *fsm) : fsm(fsm) {}
    void handleMessage(SpinMessage *msg, int inport, bool from_self);
    void sendMove(LoopBuffer path);
    void sendKillMove(LoopBuffer path);
    void sendProbeMove(LoopBuffer path);

  private:
    int get_sender_id() { return fsm->get_router()->get_id(); }
    SpinFSM *fsm;
};
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
#endif // __MEM_RUBY_NETWORK_GARNET_0_FSM_MOVE_MANAGER_HH__
