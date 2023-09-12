#ifndef __MEM_RUBY_NETWORK_GARNET_0_SPIN_MESSAGES_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_SPIN_MESSAGES_HH__
#include <map>

#include "mem/ruby/network/garnet/Spin/CommonTypes.hh"
#include "mem/ruby/network/garnet/Spin/LoopBuffer.hh"
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
        m_size = 1;
    }
    LoopBuffer path;
    const int sender_id;
    virtual SpinMessageType get_msg_type() const = 0;
    bool operator<(const SpinMessage &rhs) const;
    virtual ~SpinMessage(){};
};

class ProbeMessage : public SpinMessage
{
  public:
    ProbeMessage(int sender_id, int watch_inport)
        : SpinMessage(LoopBuffer(), sender_id),
          return_log({{watch_inport, 0}}), DEBUG_last_router(sender_id) {}
    ProbeMessage(const ProbeMessage &rhs) : SpinMessage(rhs) {
        return_log = rhs.return_log;
        DEBUG_last_router = rhs.DEBUG_last_router;
    }
    virtual SpinMessageType get_msg_type() const override { return PROBE_MSG; }
    ~ProbeMessage() override = default;
    bool return_path_ready(int inport);
    void DEBUG_set_last_router(int router_id) {
        DEBUG_last_router = router_id;
    }

  protected:
    std::map<int, unsigned int> return_log;
    int DEBUG_last_router;
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
