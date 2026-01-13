//
// Created by palulukan on 1/9/26.
//

#include "mem.h"

#include "../log.h"

#include <cassert>

DataStatus Mem::Read(ph_addr pa, bool bByte) {
  if((pa & 1) && !bByte) {
    DEBUG("MEMORY: Reading word from an odd address: 0%08o", pa);
    return DataStatus::Error(MEM_ERR_ODD_ADDR);
  }

  auto wordAddr = pa & ~1U;

  DataStatus data = 0;
  if(pa >= MEM_22BIT_UNIBUS_ADDR)
    data = _unibus->Read(wordAddr - MEM_22BIT_UNIBUS_ADDR);
  else
    data = _ram->Read(wordAddr);

  if(data.error)
    return data;

  if(bByte)
    data.data = data.data >> (8 * (pa & 1U)) & 0xFF;

  return data;
}

DataStatus Mem::Write(ph_addr pa, bool bByte, cpu_word data) {
  if((pa & 1) && !bByte) {
    DEBUG("MEMORY: Writing word to an odd address: 0%08o, data: 0%06o", pa, data);
    return DataStatus::Error(MEM_ERR_ODD_ADDR);
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

  const PartialValue pv(data, mask);

  if(pa >= MEM_22BIT_UNIBUS_ADDR)
    return _unibus->Write(pa - MEM_22BIT_UNIBUS_ADDR, pv);

  return _ram->Write(pa, pv);
}

void Mem::Reset() {
  _unibus->Reset();
}
