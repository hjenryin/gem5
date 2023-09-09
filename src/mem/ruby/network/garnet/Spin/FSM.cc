#include "mem/ruby/network/garnet/Spin/FSM.hh"

#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/OutputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"

namespace gem5 {
namespace ruby {
namespace garnet {
namespace spin {
Cycles SpinFSM::detectionThreshold = Cycles(10);
SpinFSM::SpinFSM(Router *router)
    : Consumer(router), probeMan(this), moveMan(this), m_state(OFF),
      m_router(router), sender_id(-1), is_deadlock(false) {}

void SpinFSM::flitArrive(garnet::flit *flit, int port, int vc) {
    if (m_state != OFF)
        return;
    m_state = DL_DETECT;
    this->invc = vc;
    this->inport = port;
    this->counter_start_time = curCycles();
    this->flit_watch = flit;
    schedule_wakeup(detectionThreshold);
}
void SpinFSM::flitLeave(garnet::flit *flit) {
    assert(m_state != OFF);

    if (m_state == DL_DETECT) {
        if (flit == this->flit_watch) {
            this->counter_start_time = curCycles();
            garnet::flit *new_flit = NULL;
            int new_inport, new_vc;
            for (int i = 1; i <= m_router->get_num_inports(); i++) {
                new_inport = (inport + i) % m_router->get_num_inports();
                for (int j = 0; j < m_router->get_vc_per_vnet(); j++) {
                    new_vc = (invc + j) % m_router->get_num_vcs();
                    if (m_router->getInputUnit(new_inport)
                            ->isReady(new_vc, curTick())) {
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
                this->invc = new_vc;
                schedule_wakeup(detectionThreshold);
            }
        }
    } else {
        throw std::runtime_error("Unimplemented state" + m_state);
    }
}

void SpinFSM::wakeup() {
    processSpinMessage(); // read message and
    checkTimeout();       // check timeout and transition
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
        SpinMessage *msg = msg_store.msg;
        const bool from_self = (sender_id == m_router->get_id());
        switch (msg->get_msg_type()) {

        case PROBE_MOVE_MSG:
            if (from_self) {
                assert(m_state == PROBE_MOVE);
                assert(msg->path.empty());
                m_state = FW_PROGRESS;
                is_deadlock = true;
                reset_counter(Cycles(loopBuf.size()));
                // We don't think the +1 in the paper is necessary.
            }

            break;
        case MOVE_MSG:
            auto move_msg = static_cast<MoveMessage *>(msg);
            if (from_self) {
                m_state = FW_PROGRESS;
                reset_counter(Cycles(loopBuf.size() + 1));
                is_deadlock = true;
            } else {
                assert(m_state == DL_DETECT);
                m_state = FROZEN;
                is_deadlock = true;
                reset_counter(Cycles(2 * loopBuf.size()) -
                              move_msg->spin_time);
            }
        default:
            fatal("Unimplemented message type %d", msg->get_msg_type());
        }
        moveMan.handleMessage(msg, msg_store.inport, from_self);
    }
}

void SpinFSM::checkTimeout() {
    auto time = curCycles() - counter_start_time;
    assert(time <= currentTimeoutLimit);
    if (time == currentTimeoutLimit) {
        if (m_state == DL_DETECT) {
            reset_counter(Cycles(TDD));
            probeMan.sendNewProbe(
                m_router->getInputUnit(inport)->get_outport(invc));
        } else if (m_state == MOVE) {
            reset_counter(Cycles(loopBuf.size()));
            moveMan.sendKillMove(loopBuf);
            m_state = KILL_MOVE;
        } else if (m_state == FW_PROGRESS) {
            currentTimeoutLimit = Cycles(INFINITE_);
            // TODO: SPIN
        } else if (m_state == FROZEN) {
            // TODO: execute spin
            currentTimeoutLimit = Cycles(INFINITE_);
        } else if (m_state == PROBE_MOVE) {
            reset_counter(Cycles(loopBuf.size()));
            moveMan.sendKillMove(loopBuf);
            m_state = KILL_MOVE;
        }
    }
}

void SpinFSM::sendMessage(SpinMessage *msg, int outport) {
    auto link = m_router->getOutputUnit(outport)->get_out_link();

    link->pushSpinMessage(msg);
}

void SpinFSM::reset_counter(Cycles c) {
    counter_start_time = curCycles();
    currentTimeoutLimit = c;
    schedule_wakeup(currentTimeoutLimit);
}

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
