
#ifndef __MEM_RUBY_NETWORK_GARNET_0_FSM_COMMONTYPES_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_FSM_COMMONTYPES_HH__

namespace gem5 {

namespace ruby {

namespace garnet {

namespace spin {
const int TDD = 100;

enum FSMState
{
    OFF,
    DL_DETECT,
    MOVE,
    KILL_MOVE,
    PROBE_MOVE,
    FW_PROGRESS,
    FROZEN
};
enum SpinMessageType
{
    UNNAMED,
    PROBE_MSG,
    MOVE_MSG,
    PROBE_MOVE_MSG,
    KILL_MOVE_MSG
};
enum SpecialVC { SPIN_MSG = -2, SPIN_FLIT = -1 };
// Order of the messages determines the priority of the messages
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif //__MEM_RUBY_NETWORK_GARNET_0_FSM_COMMONTYPES_HH__
