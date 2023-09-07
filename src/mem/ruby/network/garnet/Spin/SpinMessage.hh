#ifndef __MEM_RUBY_NETWORK_GARNET_0_SPIN_MESSAGES_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_SPIN_MESSAGES_HH__
#include "mem/ruby/network/garnet/Spin/CommonTypes.hh"
#include "mem/ruby/network/garnet/flit.hh"

#include
#include <queue>

namespace gem5 {

namespace ruby {

namespace garnet {
class Router;

namespace spin {
class SpinMessage : public flit
{
  public:
    SpinMessageType type;
    std::queue<PortDirection> path;
    int sender_id;
    Cycles spin_time;
    SpinMessageType get_msg_type() { return type; }
    bool operator<(const SpinMessage &rhs) const {
        if (type != rhs.type) {
            return (int)type < (int)rhs.type;
        } else {
            assert(type != KILL_MOVE_MSG && type != PROBE_MOVE_MSG);
            return getCurrentPriority(sender_id) <
                   getCurrentPriority(rhs.sender_id);
        }
    }

  private:
    int getCurrentPriority(int sender_id) const {
        return (sender_id + curTick()) / (8 * TDD);
        // MAGIC NUMBER!
    }
};
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_SPIN_MESSAGES_HH__
