#include "RpcCore.hpp"
#include "flatbuffers/flatbuffers.h"

namespace cpu_monitor {

template <typename T, size_t InitialSize = 1024>
struct RpcMsg : RpcCore::Message {
  using TableType = typename T::TableType;
  std::string serialize() const override {
    flatbuffers::FlatBufferBuilder fbb(InitialSize);
    auto offset = TableType::Pack(fbb, &msg);
    fbb.Finish(offset);
    auto data = fbb.GetBufferPointer();
    auto size = fbb.GetSize();
    return {(char*)data, size};
  }

  bool deSerialize(const std::string& data) override {
    flatbuffers::GetRoot<TableType>(data.data())->UnPackTo(&msg);
    return true;
  }

  T msg;
};

}  // namespace cpu_monitor
