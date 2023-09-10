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
    }
    LoopBuffer path;
    int sender_id;
    virtual SpinMessageType get_msg_type() const = 0;
    bool operator<(const SpinMessage &rhs) const;
    virtual ~SpinMessage(){};
};

class ProbeMessage : public SpinMessage
{
  public:
    ProbeMessage(int sender_id, int watch_inport)
        : SpinMessage({}, sender_id), return_log({{watch_inport, 0}}) {}
    ProbeMessage(const ProbeMessage &rhs) = default;
    virtual SpinMessageType get_msg_type() const override { return PROBE_MSG; }
    ~ProbeMessage() override = default;
    bool return_path_ready(int inport);

  protected:
    std::map<int, unsigned int> return_log;
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
    MoveMessage(int sender_id, LoopBuffer path, Cycles spin_time,
                int first_outport)
        : SpinMessage(path, sender_id), spin_time(spin_time),
          first_outport(first_outport) {}
    virtual SpinMessageType get_msg_type() const override { return MOVE_MSG; }
    Cycles spin_time;
    int first_outport;
    ~MoveMessage() override = default;
};
class ProbeMoveMessage : public SpinMessage
{
  public:
    ProbeMoveMessage(int sender_id, LoopBuffer path, Cycles spin_time,
                     int first_outport)
        : SpinMessage(path, sender_id), spin_time(spin_time),
          first_outport(first_outport) {}
    virtual SpinMessageType get_msg_type() const override {
        return PROBE_MOVE_MSG;
    }
    Cycles spin_time;
    int first_outport;
    ~ProbeMoveMessage() override = default;
};

} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif // __MEM_RUBY_NETWORK_GARNET_0_SPIN_MESSAGES_HH__
