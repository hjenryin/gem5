#ifndef __MEM_RUBY_NETWORK_GARNET_0_FSM_LOOPBUFFER_HH__
#define __MEM_RUBY_NETWORK_GARNET_0_FSM_LOOPBUFFER_HH__
#include <cassert>
#include <queue>

namespace gem5 {
namespace ruby {
namespace garnet {
namespace spin {
class LoopBuffer : protected std::queue<int>
{
  public:
    LoopBuffer() : std::queue<int>() {}
    int pop_front() {
        auto i = front();
        pop();
        return i;
    }
    int peek_front() {
        assert(!empty());
        return front();
    }
    void push_back(int i) { push(i); }
    int size() { return std::queue<int>::size(); }
    bool empty() { return std::queue<int>::empty(); }
    void clear() {
        while (!empty())
            pop();
    }
};
} // namespace spin
} // namespace garnet
} // namespace ruby
} // namespace gem5
#endif // __MEM_RUBY_NETWORK_GARNET_0_FSM_LOOPBUFFER_HH__
