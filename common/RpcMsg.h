#include "plugin/FlatbuffersMsg.hpp"

namespace cpu_monitor {

template <typename T, size_t InitialSize = 1024>
using RpcMsg = RpcCore::FlatbuffersMsg<T, InitialSize>;

}  // namespace cpu_monitor
