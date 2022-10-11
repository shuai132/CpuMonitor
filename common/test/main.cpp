#include "CpuMsg_generated.h"
#include "RpcMsg.h"

using namespace cpu_monitor;

int main() {
  std::string payload;
  {
    RpcMsg<msg::CpuMsgT> cpuMsg;
    for (int i = 0; i < 3; ++i) {
      auto cpuInfo = std::make_unique<msg::CpuInfoT>();
      cpuInfo->name = "cpu" + std::to_string(i);
      cpuInfo->usage = 1.f + (float)i;
      cpuMsg.msg.infos.push_back(std::move(cpuInfo));
    }

    payload = cpuMsg.serialize();
  }
  {
    RpcMsg<msg::CpuMsgT> cpuMsg;
    cpuMsg.deSerialize(payload);
    for (const auto& item : cpuMsg.msg.infos) {
      printf("name: %s, usage: %.2f\n", item->name.c_str(), item->usage);
    }
  }
}
