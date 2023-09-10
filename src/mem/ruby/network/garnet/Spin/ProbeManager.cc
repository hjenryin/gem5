#include "mem/ruby/network/garnet/Spin/ProbeManager.hh"

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
    ProbeMessage *message = new ProbeMessage(get_router_id(), watch_inport);
    message->path.push_back(outport);
    fsm->sendMessage(message, outport);
    return;
}

/**
 * @brief Handles the probe message.
 *
 * - If we aren't the sender, or the probe message returns to a new inport of
 * the sender, then the probe will be forwarded if priority check passes.
 * - If the probe message returns to a visited inport of the sender,
 * then a deadlock is detected. The path of the msg will be properly set, and a
 * move message will be sent if fsm is not already in MOVE. Returns true to
 * indicate that fsm should transition to move state.
 *
 * @return true if deadlock detected, false otherwise
 */
bool ProbeManager::handleProbe(SpinMessage *message, int inport,
                               bool from_self) {
    assert(message->get_msg_type() == PROBE_MSG);
    ProbeMessage *in_msg = static_cast<ProbeMessage *>(message);

    if (from_self) {
        if (in_msg->return_path_ready(inport)) {
            // deadlock detected, transition to move and send move message
            if (fsm->get_current_sender_ID() == -1) {
                assert(fsm->get_state() == MOVE || fsm->get_state() == FROZEN);
                fsm->set_current_sender_ID(get_router_id());
                int outport_id = message->path.pop_front();
                MoveMessage *move_msg = new MoveMessage(
                    get_router_id(), message->path,
                    Cycles(2 * message->path.size()), outport_id);
                fsm->sendMessage(move_msg, outport_id);
                delete message;
                return true;
            } else {
                delete message;
                return false;
            }
        }
    }
    if (SpinFSM::getCurrentPriority(get_router_id()) >
        SpinFSM::getCurrentPriority(in_msg->sender_id)) {
        delete message; // drop probe based on dynamic priority
        return false;
    } else {
        // forward probe message
        int num_outports = fsm->get_router()->get_num_outports();
        int num_vcs = fsm->get_router()->get_num_vcs();
        auto IU = fsm->get_router()->getInputUnit(inport);
        auto outportBools = new bool[num_outports];
        for (int outport = 0; outport < num_outports; outport++) {
            outportBools[outport] = false;
        }
        for (int invc = 0; invc < num_vcs; invc++) {
            flit *t_flit = IU->peekTopFlit(invc);
            if (t_flit == NULL) {
                delete[] outportBools;
                delete message;
                return false;
            } else {
                int outport = IU->get_outport(invc);
                auto OU = fsm->get_router()->getOutputUnit(outport);
                if (OU->get_direction() != "Local") {
                    outportBools[outport] = true;
                }
            }
        }
        for (int outport = 0; outport < num_outports; outport++) {
            if (outportBools[outport]) {
                ProbeMessage *probe_msg = new ProbeMessage(*in_msg);
                probe_msg->path.push_back(outport);
                fsm->sendMessage(probe_msg, outport);
            }
        }
        delete message;
        delete[] outportBools;
        return false;
    }
}
int ProbeManager::get_router_id() { return fsm->get_router()->get_id(); }

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
