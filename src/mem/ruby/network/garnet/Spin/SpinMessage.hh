#ifndef __MEM_RUBY_NETWORK_GARNET_0_SPIN_MESSAGES_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_SPIN_MESSAGES_HH__
#include "mem/ruby/network/garnet/Spin/CommonTypes.hh"
#include "mem/ruby/network/garnet/flit.hh"

namespace gem5 {

namespace ruby {

namespace garnet {
class Router;

namespace spin {
class SpinMessage : public flit
{
  public:
    SpinMessage(LoopBuffer path, int sender_id)
        : flit(), path(path), sender_id(sender_id) {
        m_time = curTick();
    }
    LoopBuffer path;
    int sender_id;
    virtual SpinMessageType get_msg_type() const = 0;
    bool operator<(const SpinMessage &rhs) const {
        auto type = get_msg_type();
        if (type != rhs.get_msg_type()) {
            return (int)type < (int)rhs.get_msg_type();
        } else {
            assert(type != KILL_MOVE_MSG && type != PROBE_MOVE_MSG);
            return getCurrentPriority(sender_id) <
                   getCurrentPriority(rhs.sender_id);
        }
    }
    virtual ~SpinMessage() = 0;

  private:
    int getCurrentPriority(int sender_id) const {
        return (sender_id + curTick()) / (8 * TDD);
        // MAGIC NUMBER!
    }
};

class ProbeMessage : public SpinMessage
{
  public:
    ProbeMessage(int sender_id) : SpinMessage({}, sender_id) {}
    ProbeMessage(const ProbeMessage &rhs) = default;
    virtual SpinMessageType get_msg_type() const override { return PROBE_MSG; }
    ~ProbeMessage() override = default;
};
class KillMoveMessage : public SpinMessage
{
  public:
    KillMoveMessage(int sender_id, LoopBuffer path)
        : SpinMessage(path, sender_id) {}
    virtual SpinMessageType get_msg_type() const override {
        return KILL_MOVE_MSG;
    }
    ~KillMoveMessage() override = default;
};
class MoveMessage : public SpinMessage
{
  public:
    MoveMessage(int sender_id, LoopBuffer path, Cycles spin_time)
        : SpinMessage(path, sender_id), spin_time(spin_time) {}
    virtual SpinMessageType get_msg_type() const override { return MOVE_MSG; }
    Cycles spin_time;
    ~MoveMessage() override = default;
};
class ProbeMoveMessage : public SpinMessage
{
  public:
    ProbeMoveMessage(int sender_id, LoopBuffer path, Cycles spin_time)
        : SpinMessage(path, sender_id), spin_time(spin_time) {}
    virtual SpinMessageType get_msg_type() const override {
        return PROBE_MOVE_MSG;
    }
    Cycles spin_time;
    ~ProbeMoveMessage() override = default;
};

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_SPIN_MESSAGES_HH__
