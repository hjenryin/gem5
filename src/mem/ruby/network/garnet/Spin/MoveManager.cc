#ifndef __MEM_RUBY_NETWORK_GARNET_0_SPIN_MOVE_MANAGER_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_SPIN_MOVE_MANAGER_HH__
#include "mem/ruby/network/garnet/Spin/MoveManager.hh"

#include "base/trace.hh"
#include "debug/SpinFSMDEBUG.hh"
#include "mem/ruby/network/garnet/OutputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/network/garnet/Spin/CommonTypes.hh"
#include "mem/ruby/network/garnet/Spin/SpinMessage.hh"

namespace gem5 {
namespace ruby {
namespace garnet {
namespace spin {

void MoveManager::handleMessage(SpinMessage *msg, int inport, bool from_self) {
    if (from_self) {
        DPRINTF(SpinFSMDEBUG, "Router %d received %d from self\n",
                get_router_id(), msg->get_msg_type());
    } else {
        DPRINTF(SpinFSMDEBUG, "Router %d received %d from router %d\n",
                get_router_id(), msg->get_msg_type(), msg->sender_id);
    }
    // TODO: check if msg should be dropped
    msg->set_time(curTick());
    switch (msg->get_msg_type()) {
    case MOVE_MSG: {
        auto move_msg = static_cast<MoveMessage *>(msg);
        if (from_self && move_msg->path.size() == 0) {
            // deadlock detected, do nothing in manager
            assert(fsm->get_state() == FW_PROGRESS);
        } else {
            if (!from_self) {
                assert(fsm->get_state() == FROZEN);
            }
            // strip the first direction and forward, freeze one waiting VC
            int outport_id = move_msg->path.pop_front();
            int invc =
                fsm->get_router()->find_vc_waiting_outport(inport, outport_id);
            if (invc == -1) {
                // no waiting VC, drop the message
                delete msg;
            } else {
                --move_msg->spin_time;
                fsm->sendMessage(move_msg, outport_id);
                // freeze the VC
                fsm->get_router()->toggle_freeze_vc(true, inport, invc,
                                                    outport_id);
            }
        }
    } break;
    case KILL_MOVE_MSG: {
        auto kill_move_msg = static_cast<KillMoveMessage *>(msg);
        if (!from_self) {
            // if sent by self, kill move complete, do nothing in move manager
            //  not from self, unfreeze vc (if any) and forward
            assert(fsm->get_state() == DL_DETECT || fsm->get_state() == OFF);
            int outport_id = kill_move_msg->path.pop_front();
            fsm->sendMessage(kill_move_msg, outport_id);
            // unfreeze the frozen VCs
            fsm->get_router()->toggle_freeze_vc(false);

        }
    } break;
    case PROBE_MOVE_MSG:
        // TODO
        {
            auto probe_move_msg = static_cast<ProbeMoveMessage *>(msg);
            if (from_self) {
                if (probe_move_msg->path.size() == 0) {
                    // move has returned, transition to forward progress, do
                    // nothing in manager
                } else {
                    // strip the first direction and forward
                    int outport_id = probe_move_msg->path.pop_front();
                    fsm->sendMessage(probe_move_msg, outport_id);
                }
            } else {
                assert(fsm->get_state() == FROZEN);
                // strip the first direction and forward, freeze one waiting VC
                int outport_id = probe_move_msg->path.pop_front();
                int invc = fsm->get_router()->find_vc_waiting_outport(
                    inport, outport_id);
                if (invc == -1) {
                    // no waiting VC, drop the message
                    delete msg;
                } else {
                    --probe_move_msg->spin_time;
                    fsm->sendMessage(probe_move_msg, outport_id);
                    fsm->get_router()->toggle_freeze_vc(true, inport, invc,
                                                        outport_id);
                }
            }
        }
        break;
    default:
        fatal("Unknown message type received in MoveManager. This may be a "
              "wrong ProbeMSG.\n");
    }
}

void MoveManager::sendKillMove(LoopBuffer path) {
    int outport_id = path.pop_front();
    auto msg = new KillMoveMessage(get_router_id(), path);
    fsm->sendMessage(msg, outport_id);
}
void MoveManager::sendProbeMove(LoopBuffer path) {
    int tll = path.size();
    int outport_id = path.pop_front();
    auto msg =
        new ProbeMoveMessage(get_router_id(), path, Cycles(2 * tll - 1));
    fsm->sendMessage(msg, outport_id);
}
int MoveManager::get_router_id() { return fsm->get_router()->get_id(); }

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_SPIN_MOVE_MANAGER_HH__
