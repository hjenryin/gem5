#include "mem/ruby/network/garnet/Spin/SpinMessage.hh"

#include "mem/ruby/network/garnet/Spin/FSM.hh"
#include "mem/ruby/network/garnet/Spin/LoopBuffer.hh"

namespace gem5 {
namespace ruby {
namespace garnet {
namespace spin {

bool SpinMessage::operator<(const SpinMessage &rhs) const {
    auto type = get_msg_type();
    if (type != rhs.get_msg_type()) {
        return (int)type < (int)rhs.get_msg_type();
    } else {
        assert(type != KILL_MOVE_MSG && type != PROBE_MOVE_MSG);
        return SpinFSM::getCurrentPriority(sender_id) <
               SpinFSM::getCurrentPriority(rhs.sender_id);
    }
}

/**
 * @brief This should only be called when returning to the sender. Register the
 inport, and if deadlock is detected, trim the path properly.
 *
 * @return true if deadlock detected, false otherwise
 */
bool ProbeMessage::return_path_ready(int inport) {
    if (return_log.find(inport) != return_log.end()) {
        for (int i = 0; i < return_log.at(inport); i++) {
            path.pop_front();
        }
        return true;
    } else {
        return_log.insert({inport, path.size()});
        return false;
    }
}
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
