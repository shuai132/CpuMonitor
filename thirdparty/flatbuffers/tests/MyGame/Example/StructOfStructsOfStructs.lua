--[[ MyGame.Example.StructOfStructsOfStructs

  Automatically generated by the FlatBuffers compiler, do not modify.
  Or modify. I'm a message, not a cop.

  flatc version: 22.9.29

  Declared by  : //monster_test.fbs
  Rooting type : MyGame.Example.Monster (//monster_test.fbs)

--]]

local flatbuffers = require('flatbuffers')

local StructOfStructsOfStructs = {}
local mt = {}

function StructOfStructsOfStructs.New()
  local o = {}
  setmetatable(o, {__index = mt})
  return o
end

function mt:Init(buf, pos)
  self.view = flatbuffers.view.New(buf, pos)
end

function mt:A(obj)
  obj:Init(self.view.bytes, self.view.pos + 0)
  return obj
end

function StructOfStructsOfStructs.CreateStructOfStructsOfStructs(builder, a_a_id, a_a_distance, a_b_a, a_b_b, a_c_id, a_c_distance)
  builder:Prep(4, 20)
  builder:Prep(4, 20)
  builder:Prep(4, 8)
  builder:PrependUint32(a_c_distance)
  builder:PrependUint32(a_c_id)
  builder:Prep(2, 4)
  builder:Pad(1)
  builder:PrependInt8(a_b_b)
  builder:PrependInt16(a_b_a)
  builder:Prep(4, 8)
  builder:PrependUint32(a_a_distance)
  builder:PrependUint32(a_a_id)
  return builder:Offset()
end

return StructOfStructsOfStructs