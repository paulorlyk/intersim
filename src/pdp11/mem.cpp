//
// Created by palulukan on 1/9/26.
//

#include "mem.h"

data_status_t Mem::Read(cpu_addr addr, cpu_space s, cpu_mode m, MMU* mmu) {
  auto pa = mmu ? mmu->Map(addr, s, m, false) : MMU::Map16Bit(addr);
  if(pa & MEM_HAS_ERR)
    return pa;

  if(pa >= MEM_22BIT_UNIBUS_ADDR)
    return _unibus->Read(pa - MEM_22BIT_UNIBUS_ADDR);

  return _ram->Read(pa);
}

data_status_t Mem::Write(cpu_addr addr, cpu_space s, cpu_mode m, bool bByte, cpu_word data, MMU* mmu) {
  auto pa = mmu ? mmu->Map(addr, s, m, true) : MMU::Map16Bit(addr);
  if(pa & MEM_HAS_ERR)
    return pa;

  if(pa >= MEM_22BIT_UNIBUS_ADDR)
    return _unibus->Write(pa - MEM_22BIT_UNIBUS_ADDR, bByte, data);

  return _ram->Write(pa, bByte, data);
}

void Mem::Reset() {
  _unibus->Reset();
}
