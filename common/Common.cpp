#include "Common.h"

#include "CpuMsg_generated.h"
#include "rpc_client.hpp"

using namespace cpu_monitor;

template <typename T, typename Builder>
struct CpuMsg : public RpcCore::String {
  using RpcCore::String::String;

  const T* msg() {
    auto msg = flatbuffers::GetRoot<T>(data());
    return msg;
  };

  static CpuMsg create(const std::function<void(flatbuffers::FlatBufferBuilder& fbb, Builder& builder)>& hd) {
    flatbuffers::FlatBufferBuilder fbb(1024);
    Builder builder(fbb);

    hd(fbb, builder);

    builder.Finish();

    uint8_t* buf = fbb.GetBufferPointer();
    auto size = fbb.GetSize();
    return {(char*)buf, size};
  }
};

void test() {
  auto cpuMsg = CpuMsg<msg::CpuMsg, msg::CpuMsg::Builder>::create([](flatbuffers::FlatBufferBuilder& fbb, msg::CpuMsg::Builder& builder) {
    std::vector<flatbuffers::Offset<msg::CpuInfo>> infos;
    {
      msg::CpuInfo::Builder ib(fbb);
      ib.add_name(fbb.CreateString("SS"));
      ib.add_usage(1.f);
      auto info = ib.Finish();
      infos.push_back(info);
    }
    builder.add_infos(fbb.CreateVector(infos));
  });
}
