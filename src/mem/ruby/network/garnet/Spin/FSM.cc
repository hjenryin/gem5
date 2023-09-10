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
    if (m_state == OFF) {
        m_state = DL_DETECT;
        this->invc = vc;
        this->inport = port;
        this->flit_watch = flit;
        reset_counter(detectionThreshold);
    }
}
/**
 * @brief This watches the new invc and inport if active vc found.
 */
flit *SpinFSM::roundRobinInVC(int &inport, int &invc) {
    int new_inport, new_vc;
    PortDirection inport_dir;
    flit *new_flit = NULL;
    for (int i = 1; i <= m_router->get_num_inports(); i++) {
        new_inport = (inport + i) % m_router->get_num_inports();
        for (int j = 0; j < m_router->get_vc_per_vnet(); j++) {
            new_vc = (invc + j) % m_router->get_num_vcs();
            if (m_router->getInputUnit(new_inport)
                    ->isReady(new_vc, curTick())) {
                auto IU = m_router->getInputUnit(new_inport);
                int outport = IU->get_outport(new_vc);
                auto OU = m_router->getOutputUnit(outport);
                if (OU->get_direction() != "Local") {
                    i = m_router->get_num_inports() + 1;
                    j = m_router->get_vc_per_vnet() + 1;
                    // which jumps out of both loops
                    new_flit = IU->peekTopFlit(new_vc);
                }
            }
        }
    }
    if (new_flit) {
        inport = new_inport;
        invc = new_vc;
    }
    return new_flit;
}
void SpinFSM::flitLeave(garnet::flit *flit) {
    if (m_state == DL_DETECT) {
        if (flit == this->flit_watch) {
            re_init();
        }
    }
}

void SpinFSM::wakeup() {
    assert(latest_cycle_debug <
           getCycles(curTick())); // fsm should not wake up twice in a cycle
    latest_cycle_debug = getCycles(curTick());
    if (spinning) { // special transition due the complete of spin
        spinning = false;
        assert(m_state == FROZEN || m_state == FW_PROGRESS);
        if (m_state == FW_PROGRESS) {
            m_state = PROBE_MOVE;
            reset_is_deadlock();
            reset_counter(Cycles(loopBuf.size()));
            moveMan.sendProbeMove(loopBuf);
        } else {
            re_init();
        }
    } else {
        processSpinMessage(); // read message and transition
        checkTimeout();       // check timeout and transition
    }
}
Tick SpinFSM::getTick(Cycles cycles) {
    return m_router->cyclesToTicks(cycles);
}

Cycles SpinFSM::getCycles(Tick tick) { return m_router->ticksToCycles(tick); }

void SpinFSM::registerSpinMessage(SpinMessage *msg, int inport, Tick time) {
    assert(msg != NULL);
    spin::SpinMessageType msg_type = msg->get_msg_type();
    // auto msg_stored_type = msg_store.msg->get_msg_type();
    if (msg_type == MOVE_MSG || msg_type == PROBE_MOVE_MSG) {
        if (sender_id != -1 && msg->sender_id != sender_id) {
            delete msg;
        }
    }
    if (msg_type == KILL_MOVE_MSG) {
        if (msg->sender_id != sender_id && sender_id != -1) {
            // Case 1: The fsm hasn't received the MOVE_MSG yet.
            // * but the fsm can already be reinited, and the kill move passes
            // a second time in an 8 shape. Case 2: When two loops overlaps,
            // the router may drop a move. Then it will receive the kill_move
            // of the dropped move, which should be ignored.
            delete msg;
        }
    }
    if (msg_store.msg == NULL) {
        msg_store.msg = msg;
        msg_store.inport = inport;
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
void SpinFSM::re_init() {
    reset_is_deadlock();
    // unfreeze all VCs
    m_router->toggle_freeze_vc(false);
    garnet::flit *new_flit = roundRobinInVC(inport, invc);
    if (new_flit == NULL) {
        m_state = OFF;
        reset_counter(Cycles(INFINITE_)); // init() in the paper
    } else {
        this->flit_watch = new_flit;
        reset_counter(detectionThreshold);
        m_state = DL_DETECT;
    }
}
void SpinFSM::reset_is_deadlock() {
    is_deadlock = false;
    sender_id = -1;
}
void SpinFSM::processSpinMessage() {
    if (msg_store.msg == NULL) {
        return;
    }
    SpinMessage *msg = msg_store.msg;
    int inport = msg_store.inport;
    msg_store.clear();

    const bool from_self = (sender_id == m_router->get_id());

    // Internal State transition
    switch (msg->get_msg_type()) {
    case PROBE_MSG: {
        bool deadlock_detected = probeMan.handleProbe(msg, inport, from_self);
        if (deadlock_detected) {
            // TODO infinitely running? Don't think will happen. If exist
            // some loop, there exists some probe which can detect it.
            assert(m_state == DL_DETECT);
            m_state = MOVE;
            loopBuf = msg->path;
            reset_counter(Cycles(loopBuf.size()));
        }
    }

        // probeMan was called in this case, and since all other cases
        // will call the moveMan in the end, we need to return here.
        return;
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
    case MOVE_MSG: {
        auto move_msg = static_cast<MoveMessage *>(msg);
        if (from_self) {
            if (move_msg->path.empty()) {
                m_state = FW_PROGRESS;
            }
            reset_counter(move_msg->spin_time);
            is_deadlock = true;
        } else {
            assert(m_state == DL_DETECT);
            m_state = FROZEN;
            is_deadlock = true;
            sender_id = move_msg->sender_id;
            reset_counter(move_msg->spin_time);
        }
    } break;
    case KILL_MOVE_MSG:
        reset_is_deadlock();
        if (from_self) {
            assert(m_state == KILL_MOVE);
            if (msg->path.empty()) {
                loopBuf = {};
            }
        } else {
            assert(m_state == FROZEN);
        }
        re_init();
        break;
    default:
        fatal("Unimplemented message type %d", msg->get_msg_type());
    }

    // Downstream message handling apart from fsm transitioning
    moveMan.handleMessage(msg, inport, from_self);
}

void SpinFSM::checkTimeout() {
    auto time = curCycles() - counter_start_time;
    assert(time <= currentTimeoutLimit);
    if (time == currentTimeoutLimit) {

        if (m_state == DL_DETECT) {
            reset_counter(Cycles(TDD));
            probeMan.sendNewProbe(
                m_router->getInputUnit(inport)->get_outport(invc), inport);
        } else if (m_state == MOVE) {
            reset_counter(Cycles(loopBuf.size()));
            moveMan.sendKillMove(loopBuf);
            m_state = KILL_MOVE;
        } else if (m_state == FW_PROGRESS) {
            // forward progress is for the initiating router
            currentTimeoutLimit = Cycles(INFINITE_); // stop counter
            pushSpin();
        } else if (m_state == FROZEN) {
            currentTimeoutLimit = Cycles(INFINITE_); // stop counter
            pushSpin();
        } else if (m_state == PROBE_MOVE) {
            reset_counter(Cycles(loopBuf.size()));
            moveMan.sendKillMove(loopBuf);
            m_state = KILL_MOVE;
        } else if (m_state == KILL_MOVE) {
            // This happens if a KILL_MOVE_MSG collides with a move from
            // different source_id.
            re_init();
            loopBuf = {};
        }
    }
}
// void SpinFSM::notify_spin_complete() {}
void SpinFSM::sendMessage(SpinMessage *msg, int outport) {
    auto link = m_router->getOutputUnit(outport)->get_out_link();
    link->pushSpinMessage(msg);
}
int SpinFSM::getCurrentPriority(int sender_id) {
    return (sender_id + curTick() / (8 * TDD)) %
           Router::get_total_num_routers();
    // MAGIC NUMBER!
}
void SpinFSM::reset_counter(Cycles c) {
    counter_start_time = curCycles();
    currentTimeoutLimit = c;
    m_router->schedule_wakeup(currentTimeoutLimit);
}

void SpinFSM::pushSpin() {
    m_router->spin_frozen_flit(); // TODO
    m_router->schedule_wakeup(Cycles(1));
    assert(!spinning);
    spinning = true;
}

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
