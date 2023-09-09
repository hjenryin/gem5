#include "mem/ruby/network/garnet/Spin/ProbeManager.hh"

#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/OutputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/network/garnet/Spin/FSM.hh"

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

bool ProbeManager::test_inport(int inport) {}

ProbeManager::ProbeManager(SpinFSM *fsm) : fsm(fsm) {}
void ProbeManager::sendNewProbe(int port) {
    ProbeMessage *message = new ProbeMessage(get_sender_id());
    message->path.push_back(port);
    fsm->sendMessage(message, port);
    return;
}

void ProbeManager::handleMessage(SpinMessage *message, int inport,
                                 bool from_self) {
    assert(message->get_msg_type() == PROBE_MSG);
    ProbeMessage *in_msg = static_cast<ProbeMessage *>(message);
    if (from_self) {
        // deadlock detected, transition to move and send move message
        fsm->setLoopBuf(message->path);
        int outport_id = message->path.pop_front();
        MoveMessage *move_msg = new MoveMessage(
            get_sender_id(), message->path, Cycles(2 * message->path.size()));
        fsm->sendMessage(move_msg, outport_id);
        delete message;
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
                return;
            } else {
                int outport = IU->get_outport(invc);
                outportBools[outport] = true;
            }
        }
        for (int outport = 0; outport < num_outports; outport++) {
            if (outportBools[outport]) {
                ProbeMessage *probe_msg = new ProbeMessage(*in_msg);
                probe_msg->path.push_back(outport);
                fsm->sendMessage(probe_msg, outport);
            }
        }
        delete[] outportBools;
    }
}

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
