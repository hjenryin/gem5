#include "mem/ruby/network/garnet/Spin/ProbeManager.hh"

#include "mem/ruby/network/garnet/Router.hh"
#include "mem/ruby/network/garnet/Spin/FSM.hh"

namespace gem5 {

namespace ruby {

namespace garnet {

namespace spin {
class SpinFSM;
ProbeManager::ProbeManager(Router *router) : router(router) {}
void ProbeManager::send(SpinMessage *message, int port) {
    auto link = router->getOutputUnit(port)->getLink();
}

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
