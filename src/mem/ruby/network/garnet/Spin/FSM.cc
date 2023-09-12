#include "mem/ruby/network/garnet/Spin/FSM.hh"

#include "base/trace.hh"
#include "debug/SpinFSMDEBUG.hh"
#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/OutputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"

namespace gem5 {
namespace ruby {
namespace garnet {
namespace spin {
SpinFSM::SpinFSM(Router *router, bool spin_enabled)
    : probeMan(this), moveMan(this), m_state(OFF), m_router(router),
      mover_id(-1), is_deadlock(false), spin_enabled(spin_enabled),
      currentTimeoutLimit(Cycles(INFINITE_)) {}

void SpinFSM::flitArrive(garnet::flit *flit, int port, int vc) {
    if (!spin_enabled)
        return;
    if (m_router->getInportDirection(port) == "Local") {
        return;
    }
    if (m_state == OFF) {
        m_state = DL_DETECT;
        this->invc = vc;
        this->inport = port;
        this->flit_watch = flit;
        reset_counter(Cycles(TDD));
    }
}
/**
 * @brief This watches the new invc and inport if active vc found.
 */
flit *SpinFSM::roundRobinInVC(int &inport, int &invc) {
    int new_inport, new_vc;
    PortDirection inport_dir;
    flit *new_flit = NULL;
    const int num_inports = m_router->get_num_inports();
    const int num_vcs = m_router->get_vc_per_vnet();
    for (int i = 1; i <= num_inports; i++) {
        new_inport = (inport + i) % num_inports;
        if (m_router->getInportDirection(new_inport) == "Local")
            continue;
        for (int j = 0; j < num_vcs; j++) {
            new_vc = (invc + j) % num_vcs;
            if (m_router->getInputUnit(new_inport)
                    ->isReady(new_vc, curTick())) {
                auto IU = m_router->getInputUnit(new_inport);
                int outport = IU->get_outport(new_vc);
                auto OU = m_router->getOutputUnit(outport);
                if (OU->get_direction() != "Local") {
                    i = num_inports + 1;
                    j = num_vcs + 1;
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
    if (msg_store.msg == NULL) {
        DPRINTF(SpinFSMDEBUG,
                "SpinFSM at router %d with state %d woke up with empty "
                "msg_store.\n",
                m_router->get_id(), m_state);
    } else {
        DPRINTF(
            SpinFSMDEBUG,
            "SpinFSM at router %d with state %d woke up with msg type %d.\n",
            m_router->get_id(), m_state, msg_store.msg->get_msg_type());
    }

    msg_sent_this_cycle.clear();
    assert(latest_cycle_debug <
           getCycles(curTick())); // fsm should not wake up twice in a cycle
    latest_cycle_debug = getCycles(curTick());

    if (spinning == 1) {
        // special transition due the complete of spin
        spinning = 0;
        if (msg_store.msg != NULL) {
            std::cout << "Drop a " << msg_store.msg->get_msg_type()
                      << " msg because spinning has just set to 0."
                      << std::endl;
            msg_store.clear();
        }
        assert(m_state == FROZEN || m_state == FW_PROGRESS);
        if (m_state == FW_PROGRESS) {
            // check there is still a vc waiting on first outport
            int first_outport = getLoopBuf().peek_front();
            int inport = m_router->get_frozen_inport(first_outport);
            m_router->toggle_freeze_vc(false);
            int invc =
                m_router->find_vc_waiting_outport(inport, first_outport);
            if (invc == -1) {
                re_init(); // no vc waiting, abort with no kill_move sent
            } else {
                m_router->toggle_freeze_vc(true, inport, invc, first_outport);
                m_state = PROBE_MOVE;
                reset_counter(Cycles(loopBuf.size()));
                moveMan.sendProbeMove(loopBuf);
            }
        } else {
            if (msg_store.msg != NULL) {
                std::cout << "Drop a " << msg_store.msg->get_msg_type()
                          << " msg because spinning has just set to 1."
                          << std::endl;
                msg_store.clear();
            }
            re_init();
        }
    } else if (spinning == 2) {
        spinning--;
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
        if (mover_id != -1 && msg->sender_id != mover_id) {
            delete msg;
        }
    }
    if (msg_type == KILL_MOVE_MSG) {
        if (msg->sender_id != mover_id && mover_id != -1) {
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
        reset_counter(Cycles(TDD));
        m_state = DL_DETECT;
    }
}
void SpinFSM::reset_is_deadlock() {
    is_deadlock = false;
    mover_id = -1;
}

void SpinFSM::processSpinMessage() {
    if (msg_store.msg == NULL) {
        return;
    }
    auto [msg, inport] = msg_store;
    msg_store.clear();

    const bool from_self = (msg->sender_id == m_router->get_id());

    // Internal State transition
    switch (msg->get_msg_type()) {
    case PROBE_MSG: {
        if (m_state != OFF && m_state != DL_DETECT) {
            delete msg;
        } else {
            bool deadlock_detected =
                probeMan.handleProbe(msg, inport, from_self);
            if (deadlock_detected) {
                // TODO infinitely running? Don't think will happen. If exist
                // some loop, there exists some probe which can detect it.
                assert(m_state == DL_DETECT);
                m_state = MOVE;
                loopBuf = msg->path;
                int first_outport = loopBuf.peek_front();
                assert(first_outport <= 15 && first_outport >= 0);
                reset_counter(Cycles(loopBuf.size()));
            }
            delete msg;
        }
    }

        // probeMan was called in this case, and since all other cases
        // will call the moveMan in the end, we need to return here.
        return;
    case PROBE_MOVE_MSG: {
        auto move_msg = static_cast<ProbeMoveMessage *>(msg);
        if (from_self) {
            assert(m_state == PROBE_MOVE);
            if (move_msg->path.empty()) {
                m_state = FW_PROGRESS; // deadlock confirmed
            }
            is_deadlock = true;
            reset_counter(Cycles(loopBuf.size()));
            // We don't think the +1 in the paper is necessary.
        } else {
            assert(m_state == DL_DETECT || m_state == OFF);
            m_state = FROZEN;
            is_deadlock = true;
            mover_id = move_msg->sender_id;
            reset_counter(move_msg->spin_time);
        }
    } break;
    case MOVE_MSG: {
        auto move_msg = static_cast<MoveMessage *>(msg);
        if (from_self) {
            if (move_msg->path.empty()) {
                m_state = FW_PROGRESS; // deadlock confirmed
            }
            reset_counter(move_msg->spin_time);
            is_deadlock = true;
        } else {
            assert(m_state == DL_DETECT || m_state == OFF);
            m_state = FROZEN;
            is_deadlock = true;
            mover_id = move_msg->sender_id;
            reset_counter(move_msg->spin_time);
        }
    } break;
    case KILL_MOVE_MSG:
        reset_is_deadlock();
        if (from_self) {
            assert(m_state == KILL_MOVE);
            if (msg->path.empty()) {
                loopBuf.clear();
            }
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
        DPRINTF(SpinFSMDEBUG,
                "SpinFSM at router %d with state %d timed out.\n",
                m_router->get_id(), m_state);
        if (m_state == DL_DETECT) {
            reset_counter(Cycles(TDD));
            int outport = m_router->getInputUnit(inport)->get_outport(invc);
            if (msg_sent_this_cycle.find(outport) ==
                msg_sent_this_cycle.end()) {
                probeMan.sendNewProbe(outport, inport);
            }

        } else {
            assert(msg_sent_this_cycle.empty());
            if (m_state == MOVE) {
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
                loopBuf.clear();
            }
        }
    }
}
void SpinFSM::registerOutport(int outport) {
    assert(msg_sent_this_cycle.find(outport) == msg_sent_this_cycle.end());
    msg_sent_this_cycle.insert(outport);
}
void SpinFSM::sendMessage(SpinMessage *msg, int outport) {
    assert(msg->get_time() == curTick());
    registerOutport(outport);
    auto OU = m_router->getOutputUnit(outport);
    OU->get_out_link()->pushSpinMessage(msg);
    DPRINTF(SpinFSMDEBUG,
            "SpinFSM at router %d with state %d sent msg type %d to outport "
            "%s (%d).\n",
            m_router->get_id(), m_state, msg->get_msg_type(),
            OU->get_direction(), outport);
}
int SpinFSM::getCurrentPriority(int sender_id) {
    return (sender_id + curTick() / (4 * TDD)) %
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
    m_router->schedule_wakeup(Cycles(2));
    assert(!spinning);
    spinning = 2;
}

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
