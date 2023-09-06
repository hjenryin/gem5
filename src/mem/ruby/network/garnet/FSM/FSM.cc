#include "mem/ruby/network/garnet/FSM/FSM.hh"

#include "mem/ruby/network/garnet/InputUnit.hh"
#include "mem/ruby/network/garnet/Router.hh"

namespace gem5 {
namespace ruby {
namespace garnet {
namespace fsm {
Cycles SpinFSM::detectionThreshold = Cycles(10);
SpinFSM::SpinFSM(Router *router)
    : Consumer(router), m_state(OFF), m_router(router) {}
void SpinFSM::flitArrive(garnet::flit *flit, int port, int vc, Tick time) {
    if (m_state != OFF)
        return;
    m_state = DL_DETECT;
    this->vc = vc;
    this->inport = port;
    this->time = time;
    this->flit = flit;
    schedule_wakeup(detectionThreshold);
}
void SpinFSM::flitLeave(garnet::flit *flit, Tick time) {
    assert(m_state != OFF);

    if (m_state == DL_DETECT) {
        if (flit == this->flit) {
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
                this->flit = new_flit;
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
    if (m_state == OFF)
        return;
    if ((m_state == DL_DETECT) &&
        (curTick() - time >= getTick(detectionThreshold))) {
        // deadlock detected
        }
}

} // namespace fsm
} // namespace garnet
} // namespace ruby
} // namespace gem5
