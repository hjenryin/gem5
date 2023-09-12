#include "mem/ruby/network/garnet/Spin/ProbeManager.hh"

#include "base/trace.hh"
#include "debug/SpinFSMDEBUG.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/OutputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/network/garnet/Spin/FSM.hh"
#include "mem/ruby/network/garnet/Spin/SpinMessage.hh"

namespace gem5 {

namespace ruby {

namespace garnet {

namespace spin {

bool ProbeManager::test_outport(int inport, int invc, int outport) {
    Router *router = fsm->get_router();
    int vnet = invc / router->get_vc_per_vnet();
    bool has_credit = false;

    auto output_unit = router->getOutputUnit(outport);

    if (output_unit->has_free_vc(vnet)) {
        has_credit = true;
    }
    return has_credit;
}

ProbeManager::ProbeManager(SpinFSM *fsm) : fsm(fsm) {}

void ProbeManager::sendNewProbe(int outport, int watch_inport) {
    assert(outport <= 15 && outport >= 0);
    ProbeMessage *message = new ProbeMessage(get_router_id(), watch_inport);
    message->path.push_back(outport);
    fsm->sendMessage(message, outport);
    return;
}
/**
 * @return int Sent outport Id.
 */
void ProbeManager::sendMove(LoopBuffer path) {

    int tll = path.size();
    int outport_id = path.pop_front();
    auto msg = new MoveMessage(get_router_id(), path, Cycles(2 * tll - 1));
    fsm->sendMessage(msg, outport_id);
}
/**
 *  Handles the probe message. Message is not deleted incase we need it
 * later.
 *
 * @brief - If we aren't the sender, or the probe message returns to a new
 * inport of the sender, then the probe will be forwarded if priority check
 * passes.
 * @brief - If the probe message returns to a visited inport of the sender,
 * then a deadlock is detected. The path of the msg will be properly set, and a
 * move message will be sent if fsm is not already in MOVE.
 * @brief - Returns true to
 * indicate that fsm should transition to move state.
 *
 * @return true if deadlock detected, false otherwise
 */
bool ProbeManager::handleProbe(SpinMessage *message, int inport,
                               bool from_self) {
    if (from_self) {
        DPRINTF(SpinFSMDEBUG, "Router %d received probe from self\n",
                get_router_id());
    } else {
        DPRINTF(SpinFSMDEBUG, "Router %d received probe from %d\n",
                get_router_id(), message->sender_id);
    }
    message->set_time(curTick());
    assert(message->get_msg_type() == PROBE_MSG);
    ProbeMessage *in_msg = static_cast<ProbeMessage *>(message);

    if (from_self) {
        if (in_msg->return_path_ready(inport)) {
            // deadlock detected, transition to move and send move message
            // also check there is still a vc waiting on first outport in path
            int first_outport = in_msg->path.peek_front();
            int vc = fsm->get_router()->find_vc_waiting_outport(inport,
                                                                first_outport);
            if (fsm->get_mover_ID() == -1 && vc != -1) {
                assert(fsm->get_state() == DL_DETECT);
                fsm->set_current_sender_ID(get_router_id());
                sendMove(in_msg->path);
                fsm->get_router()->toggle_freeze_vc(true, inport, vc,
                                                    first_outport);
                return true;
            } else {
                return false;
            }
        }
    }
    if (SpinFSM::getCurrentPriority(get_router_id()) >
        SpinFSM::getCurrentPriority(in_msg->sender_id)) {
        // drop probe based on dynamic priority
        return false;
    } else {
        // forward probe message
        int num_outports = fsm->get_router()->get_num_outports();
        int num_vcs = fsm->get_router()->get_vc_per_vnet();
        auto IU = fsm->get_router()->getInputUnit(inport);
        auto outportBools = new bool[num_outports];
        for (int outport = 0; outport < num_outports; outport++) {
            outportBools[outport] = false;
        }
        for (int invc = 0; invc < num_vcs; invc++) {
            flit *t_flit = IU->peekTopFlit(invc);
            if (t_flit == NULL) {
                delete[] outportBools;
                return false;
            } else {
                int outport = IU->get_outport(invc);
                auto OU = fsm->get_router()->getOutputUnit(outport);
                if (OU->get_direction() != "Local") {
                    outportBools[outport] = true;
                } else {
                    delete[] outportBools;
                    return false;
                    // flits at local port will be consumed next cycle
                }
            }
        }
        for (int outport = 0; outport < num_outports; outport++) {
            if (outportBools[outport]) {
                ProbeMessage *probe_msg = new ProbeMessage(*in_msg);
                probe_msg->DEBUG_set_last_router(get_router_id());
                probe_msg->path.push_back(outport);
                fsm->sendMessage(probe_msg, outport);
            }
        }
        delete[] outportBools;
        return false;
    }
}
int ProbeManager::get_router_id() { return fsm->get_router()->get_id(); }

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
