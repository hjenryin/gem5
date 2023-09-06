
#ifndef __MEM_RUBY_NETWORK_GARNET_0_FSM_COMMONTYPES_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_FSM_COMMONTYPES_HH__

#include "mem/ruby/common/NetDest.hh"

namespace gem5 {

namespace ruby {

namespace garnet {

namespace fsm {
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
} // namespace fsm
} // namespace garnet
} // namespace ruby
} // namespace gem5

#endif //__MEM_RUBY_NETWORK_GARNET_0_FSM_COMMONTYPES_HH__
