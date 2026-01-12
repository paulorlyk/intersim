//
// Created by palulukan on 1/11/26.
//

#include "unibus.h"

#include "../log.h"

#include <cassert>
#include <cstring>

void RAM::Poke(ph_addr base, const uint8_t *buf, ph_size size) {
  assert((base + size) < MEM_SIZE_BYTES);

  memcpy((uint8_t *)_mem + base, buf, size);
}

data_status_t RAM::Read(ph_addr addr) {
  assert((addr & 1) == 0);

  if(addr >= MEM_SIZE_BYTES)
    return MEM_ERR(MEM_ERR_NX_MEM);

  return _mem[addr >> 1];
}

data_status_t RAM::Write(ph_addr addr, cpu_word data, cpu_word mask) {
  assert((addr & 1) == 0);

  if(addr >= MEM_SIZE_BYTES)
    return MEM_ERR(MEM_ERR_NX_MEM);

  _mem[addr >> 1] = UnibusDevice::AugmentData(data, _mem[addr >> 1], mask);

  return 0;
}

cpu_word UnibusDevice::AugmentData(cpu_word data, cpu_word val, cpu_word mask) {
  return (data & mask) | (val & ~mask);
}

data_status_t Unibus::Read(un_addr addr) {
  assert((addr & 1) == 0);

  assert(addr <= MEM_UNIBUS_ADDR_MAX);    // TODO: Debug
  addr &= MEM_UNIBUS_ADDR_MAX;

  if(addr < MEM_UNIBUS_PERIPH_PAGE_ADDR)
    return _ram->Read(addr);

  //DEBUG("UNIBUS: I/O read: 0%06o", addr);

  auto io = _getIoHandler(addr);
  if(!io) {
    DEBUG("UNIBUS: Reading unknown periphery page address: 0%08o", addr);
    return MEM_ERR(MEM_ERR_UNB_TIMEOUT);
  }

  return io->Read(addr);
}

data_status_t Unibus::Write(un_addr addr, cpu_word data, cpu_word mask) {
  assert((addr & 1) == 0);

  assert(addr <= MEM_UNIBUS_ADDR_MAX);    // TODO: Debug
  addr &= MEM_UNIBUS_ADDR_MAX;

  if(addr < MEM_UNIBUS_PERIPH_PAGE_ADDR)
    return _ram->Write(addr, data, mask);

  //DEBUG("UNIBUS: I/O write: 0%06o -> addr 0%06o", data, addr);

  auto io = _getIoHandler(addr);
  if(!io) {
    DEBUG("UNIBUS: Writing unknown periphery page address: 0%08o, data: 0%06o", addr, data);
    return MEM_ERR(MEM_ERR_UNB_TIMEOUT);
  }

  io->Write(addr, data, mask);

  return 0;
}

void Unibus::RegisterDevice(UnibusDevice* dev) {
  assert(dev);

  _devices.push_back(dev);

  NotifyIRQ(dev);

  for(auto &io : dev->_ioMap) {
    auto ioStart = io.start;
    auto ioEnd = io.start + io.size;

    assert((ioStart & 1) == 0);
    assert((ioEnd & 1) == 0);
    assert(ioStart >= MEM_UNIBUS_PERIPH_PAGE_ADDR);
    assert(ioEnd >= MEM_UNIBUS_PERIPH_PAGE_ADDR);
    assert(ioStart <= MEM_UNIBUS_ADDR_MAX);
    assert(ioEnd <= MEM_UNIBUS_ADDR_MAX + 1);

    ioStart = (ioStart - MEM_UNIBUS_PERIPH_PAGE_ADDR) >> 1;
    ioEnd = (ioEnd - MEM_UNIBUS_PERIPH_PAGE_ADDR) >> 1;

    for(un_addr i = ioStart; i < ioEnd; ++i) {
      assert(!_peripheralPageMap[i]);

      _peripheralPageMap[i] = dev;
    }
  }
}

void Unibus::UnregisterDevice(UnibusDevice* dev) {
  assert(dev);

  for(auto &io : dev->_ioMap) {
    auto ioStart = io.start;
    auto ioEnd = io.start + io.size;

    assert((ioStart & 1) == 0);
    assert((ioEnd & 1) == 0);
    assert(ioStart >= MEM_UNIBUS_PERIPH_PAGE_ADDR);
    assert(ioEnd >= MEM_UNIBUS_PERIPH_PAGE_ADDR);
    assert(ioStart <= MEM_UNIBUS_ADDR_MAX);
    assert(ioEnd <= MEM_UNIBUS_ADDR_MAX + 1);

    ioStart = (ioStart - MEM_UNIBUS_PERIPH_PAGE_ADDR) >> 1;
    ioEnd = (ioEnd - MEM_UNIBUS_PERIPH_PAGE_ADDR) >> 1;

    for(un_addr i = ioStart; i < ioEnd; ++i)
      _peripheralPageMap[i] = nullptr;
  }

  for(auto it = _devices.begin(); it != _devices.begin(); ++it) {
    if(*it == dev) {
      _devices.erase(it);
      break;
    }
  }

  _updateCurrIRQDevice();
}

void Unibus::NotifyIRQ(UnibusDevice *dev) {
  assert(dev);

  if(dev->_bIRQ && _currIRQDevice && _currIRQDevice->_irqPriority > dev->_irqPriority)
    return;

  if(!dev->_bIRQ && _currIRQDevice != dev)
    return;

  _updateCurrIRQDevice();
}

UnibusDevice * Unibus::GetIRQ(int minPriority) const {
  if(_currIRQDevice && _currIRQDevice->_irqPriority >= minPriority)
    return _currIRQDevice;

  return nullptr;
}

cpu_word Unibus::AckIRQ() {
  assert(_currIRQDevice);

  if(!_currIRQDevice)
    return 0;

  auto dev = _currIRQDevice;

  _updateCurrIRQDevice();

  return dev->IrqAck();
}

void Unibus::Reset() {
  for(auto dev : _devices)
    dev->Reset();

  _updateCurrIRQDevice();
}

UnibusDevice* Unibus::_getIoHandler(un_addr addr) const {
  assert((addr & 1) == 0);
  assert(addr <= MEM_UNIBUS_ADDR_MAX);

  auto idx = (addr - MEM_UNIBUS_PERIPH_PAGE_ADDR) >> 1;
  return _peripheralPageMap[idx];
}

void Unibus::_updateCurrIRQDevice() {
  _currIRQDevice = nullptr;

  for(auto d : _devices) {
    if(d->_bIRQ && (!_currIRQDevice || _currIRQDevice->_irqPriority < d->_irqPriority)) {
      _currIRQDevice = d;

      if(_currIRQDevice->_irqPriority == IRQ_PRIORITY_MAX)
        break;
    }
  }
}
