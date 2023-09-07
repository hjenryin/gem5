#include "mem/ruby/network/garnet/Spin/FSM.hh"

#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"

namespace gem5 {
namespace ruby {
namespace garnet {
namespace spin {
Cycles SpinFSM::detectionThreshold = Cycles(10);
SpinFSM::SpinFSM(Router *router)
    : Consumer(router), pm(router), m_state(OFF), m_router(router),
      sender_id(-1), is_deadlock(false) {}
void SpinFSM::flitArrive(garnet::flit *flit, int port, int vc, Tick time) {
    if (m_state != OFF)
        return;
    m_state = DL_DETECT;
    this->vc = vc;
    this->inport = port;
    this->time = time;
    this->flit_watch = flit;
    schedule_wakeup(detectionThreshold);
}
void SpinFSM::flitLeave(garnet::flit *flit, Tick time) {
    assert(m_state != OFF);

    if (m_state == DL_DETECT) {
        if (flit == this->flit_watch) {
            this->time = time;
            garnet::flit *new_flit = NULL;
            int new_inport, new_vc;
            for (int i = 1; i <= m_router->get_num_inports(); i++) {
                new_inport = (inport + i) % m_router->get_num_inports();
                for (int j = 0; j < m_router->get_vc_per_vnet(); j++) {
                    new_vc = (vc + j) % m_router->get_num_vcs();
                    if (m_router->getInputUnit(new_inport)
                            ->isReady(new_vc, time)) {
                        i = m_router->get_num_inports() + 1;
                        j = m_router->get_vc_per_vnet() + 1;
                        new_flit = m_router->getInputUnit(new_inport)
                                       ->peekTopFlit(new_vc);
                        // which jumps out of both loops
                    }
                }
            }
            if (new_flit == NULL) {
                // no flit is ready
                m_state = OFF;
            } else {
                // flit is ready
                this->flit_watch = new_flit;
                this->inport = new_inport;
                this->vc = new_vc;
                schedule_wakeup(detectionThreshold);
            }
        }
    } else {
        throw std::runtime_error("Unimplemented state" + m_state);
    }
}

void SpinFSM::wakeup() {
    if (m_state == OFF) {
    } else if ((m_state == DL_DETECT) &&
               (curTick() - time >= getTick(detectionThreshold))) {
        // deadlock detected
    }
    processSpinMessage();
}
Tick SpinFSM::getTick(Cycles cycles) {
    return m_router->cyclesToTicks(cycles);
}
Cycles SpinFSM::getCycles(Tick tick) { return m_router->ticksToCycles(tick); }
void SpinFSM::registerSpinMessage(SpinMessage *msg, int inport, Tick time) {
    assert(msg != NULL);
    spin::SpinMessageType msg_type = msg->get_msg_type();
    auto msg_stored_type = msg_store.msg->get_msg_type();
    if (msg_type == MOVE_MSG) {
        if (sender_id != -1 || is_deadlock) {
            delete msg;
        }
    }
    if (msg_type == KILL_MOVE_MSG) {
        if (msg->sender_id != sender_id) {
            delete msg;
        }
    }
    if (msg_store.msg == NULL) {
        msg_store.msg = msg;
        return;
    }

    if (*msg_store.msg < *msg) {
        delete msg_store.msg;
        msg_store.msg = msg;
        msg_store.inport = inport;
    } else {
        delete msg;
    }
}
void SpinFSM::processSpinMessage() {
    if (msg_store.msg == NULL) {
        return;
    } else {
        bool from_self = sender_id == m_router->get_id();
        switch (msg_store.msg->type) {

        case PROBE_MOVE_MSG:
            if (from_self) {
                assert(m_state == PROBE_MOVE);
                assert(msg_store.msg->path.empty());
                m_state = FW_PROGRESS;

            } else {
                mm.handleMessage(msg_store.msg, msg_store.inport);
            }
            break;
        case MOVE_MSG:

        default:
            break;
        }
    }
}

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
