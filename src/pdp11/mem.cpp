//
// Created by palulukan on 1/9/26.
//

#include "mem.h"

#include "../log.h"

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

  if((pa & 1) && !bByte) {
    DEBUG("MEMORY: Writing word to an odd address: 0%08o, data: 0%06o", pa, data);
    return MEM_ERR(MEM_ERR_ODD_ADDR);
  }

  cpu_word mask = 0xFFFF;
  if(bByte) {
    if(pa & 1) {
      pa -= 1;
      data <<= 8;
      mask = 0xFF00;
    } else {
      mask = 0x00FF;
    }
  }

  if(pa >= MEM_22BIT_UNIBUS_ADDR)
    return _unibus->Write(pa - MEM_22BIT_UNIBUS_ADDR, data, mask);

  return _ram->Write(pa, data, mask);
}

void Mem::Reset() {
  _unibus->Reset();
}
