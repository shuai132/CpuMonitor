#include "Common.h"

#include "CpuMsg_generated.h"
#include "ProgressMsg_generated.h"
#include "RpcMsg.h"

namespace cpu_monitor {

void _test() {
  RpcMsg<msg::CpuMsgT> cpmMsg;
  RpcMsg<msg::ProgressMsgT> progressMsg;
}

}  // namespace cpu_monitor
