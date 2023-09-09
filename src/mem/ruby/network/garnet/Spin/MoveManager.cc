#ifndef __MEM_RUBY_NETWORK_GARNET_0_SPIN_MOVE_MANAGER_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_SPIN_MOVE_MANAGER_HH__
#include "mem/ruby/network/garnet/Spin/MoveManager.hh"

#include "mem/ruby/network/garnet/OutputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/network/garnet/Spin/CommonTypes.hh"
#include "mem/ruby/network/garnet/Spin/SpinMessage.hh"

namespace gem5 {
namespace ruby {
namespace garnet {
namespace spin {

void MoveManager::handleMessage(SpinMessage *msg, int inport, bool from_self) {
    // TODO: check if msg should be dropped
    switch (msg->get_msg_type()) {
    case MOVE_MSG:
        auto move_msg = static_cast<MoveMessage *>(msg);
        if (from_self) {
            // sent by self, a move has returned, transitioned to forward
            // progress, do nothing in MoveManager
        } else {
            assert(fsm->get_state() == DL_DETECT);
            // strip the first direction and forward, freeze one waiting VC
            int outport_id = move_msg->path.pop_front();
            --move_msg->spin_time;
            fsm->sendMessage(move_msg, outport_id);
        }
        break;
    case KILL_MOVE_MSG:
        // TODO
        break;
    case PROBE_MOVE_MSG:
        // TODO
        break;
    }
}

void MoveManager::sendMove(LoopBuffer path) {

    int tll = path.size();
    int outport_id = path.pop_front();
    auto msg = new MoveMessage(get_sender_id(), path, Cycles(2 * tll));
    fsm->sendMessage(msg, outport_id);
}
void MoveManager::sendKillMove(LoopBuffer path) {
    int outport_id = path.pop_front();
    auto msg = new KillMoveMessage(get_sender_id(), path);
    fsm->sendMessage(msg, outport_id);
}
void MoveManager::sendProbeMove(LoopBuffer path) {
    int tll = path.size();
    int outport_id = path.pop_front();
    auto msg = new ProbeMoveMessage(get_sender_id(), path, Cycles(tll));
    fsm->sendMessage(msg, outport_id);
}
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_SPIN_MOVE_MANAGER_HH__
