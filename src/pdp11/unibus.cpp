//
// Created by palulukan on 1/11/26.
//

#include "unibus.h"

#include "../log.h"

#include <cassert>
#include <cstring>
#include <optional>

void RAM::Poke(ph_addr base, const uint8_t *buf, ph_size size) {
  assert((base + size) < MEM_SIZE_BYTES);

  memcpy((uint8_t *)_mem + base, buf, size);
}

bool RAM::LoadFile(const char *imageFileName, ph_addr base) {
  if(base >= MEM_SIZE_BYTES)  {
    DEBUG("RAM: Base is to high");
    return false;
  }

  return _readFile(imageFileName, (uint8_t*)_mem + base, MEM_SIZE_BYTES - base) >= 0;
}

std::optional<cpu_addr> RAM::LoadTape(const char *imageFileName, ph_addr base) {
  /*
    Data on tapes in the "Absolute Format" is organized into blocks; each block begins with
    a 6-byte header:

        2-byte signature (0x0001)
        2-byte block length (N + 6, because it includes the 6-byte header)
        2-byte load address

    followed by N data bytes.  If N is zero, then the 2-byte load address is the exec address,
    unless the address is odd (usually 1).  DEC's Absolute Loader jumps to the exec address
    in former case, halts in the latter.

    All values are stored "little endian" (low byte followed by high byte), just like the
    PDP-11's memory architecture.

    After the data bytes, there is a single checksum byte.  The 8-bit sum of all the bytes in
    the block (including the header bytes and checksum byte) should be zero.
  */
  std::vector<uint8_t> buf(MEM_SIZE_BYTES);

  auto read = _readFile(imageFileName, buf.data(), buf.size());
  if(read < 0)
    return std::nullopt;

  enum {
    signature1,
    signature2,
    block_len1,
    block_len2,
    addr1,
    addr2,
    data,
    checksum,
  } state = signature1;

  uint16_t blockLen = 0;
  uint16_t loadAddr = 0;
  uint16_t exexAddr = 0;
  uint8_t checkSum = 0;
  for(ssize_t i = 0; i < read; ++i) {
    auto b = buf[i];

    checkSum += b;

    switch(state) {
      case signature1: {
        if(b == 1) {
          state = signature2;
          break;
        }

        if(b != 0) {
          ERROR("Invalid byte 0x%02x at position %ld while looking for signature", b, i);
          return std::nullopt;
        }
        break;
      }

      case signature2: {
        if(b != 0) {
          ERROR("Invalid byte 0x%02x at position %ld while looking for signature", b, i);
          return std::nullopt;
        }

        state = block_len1;
        break;
      }

      case block_len1: {
        blockLen = b;
        state = block_len2;
        break;
      }

      case block_len2: {
        blockLen |= (uint16_t)b << 8;

        if(blockLen < 6) {
          ERROR("Block length is too small");
          return std::nullopt;
        }
        blockLen -= 6;  // Header length

        state = addr1;
        break;
      }

      case addr1: {
        if(blockLen)
          loadAddr = b;
        else
          exexAddr = b;

        state = addr2;
        break;
      }

      case addr2: {
        if(blockLen)
          loadAddr |= (uint16_t)b << 8;
        else
          exexAddr |= (uint16_t)b << 8;

        state = data;
        break;
      }

      case data: {
        if(!blockLen) {
          state = checksum;
          break;
        }

        if(loadAddr >= MEM_SIZE_BYTES) {
          ERROR("Load address is outside the memry");
          return std::nullopt;
        }

        ((uint8_t *)_mem)[loadAddr++] = b;
        blockLen--;
        break;
      }

      case checksum: {
        if(checkSum) {
          ERROR("Invalid block checksum");
          return std::nullopt;
        }

        blockLen = 0;
        loadAddr = 0;

        state = signature1;
        break;
      }
    }
  }

  if(state != signature1) {
    ERROR("Not enough data in the block");
    return std::nullopt;
  }

  return loadAddr;
}

bool RAM::DumpToFile(const char *fileName) {
  if(!fileName) {
    DEBUG("RAM: Need file name");
    return false;
  }

  FILE *pfImage = fopen(fileName, "wb");
  if(!pfImage) {
    DEBUG("RAM: Failed to open file `%s`", fileName);
    return false;
  }

  for(size_t written = 0; written < (size_t)MEM_SIZE_BYTES;) {
    auto w = fwrite((uint8_t *)_mem + written, 1, std::min((size_t)4096, (size_t)MEM_SIZE_BYTES - written), pfImage);
    written += w;

    if(!w) {
      ERROR("Failed to write the image file");
      fclose(pfImage);
      return false;
    }
  }

  fclose(pfImage);
  return true;
}

DataStatus RAM::Read(ph_addr addr) {
  assert((addr & 1) == 0);

  if(addr >= MEM_SIZE_BYTES)
    return DataStatus::Error(MEM_ERR_NX_MEM);

  return _mem[addr >> 1];
}

DataStatus RAM::Write(ph_addr addr, const PartialValue& data) {
  assert((addr & 1) == 0);

  if(addr >= MEM_SIZE_BYTES)
    return DataStatus::Error(MEM_ERR_NX_MEM);

  _mem[addr >> 1] = data.GetValue(_mem[addr >> 1]);

  return 0;
}

ssize_t RAM::_readFile(const char *imageFileName, uint8_t *buf, size_t size) {
  if(!imageFileName) {
    DEBUG("RAM: Need image file name");
    return -1;
  }

  FILE *pfImage = fopen(imageFileName, "rb");
  if(!pfImage) {
    DEBUG("RAM: Failed to open image file `%s`", imageFileName);
    return -1;
  }

  size_t nRead = 0;
  while(nRead < size) {
    nRead += fread(buf + nRead, 1, size - nRead, pfImage);

    if(feof(pfImage))
      break;

    if(ferror(pfImage)) {
      DEBUG("RAM: Error reading the image file `%s`", imageFileName);

      fclose(pfImage);
      return -1;
    }
  }

  fclose(pfImage);

  DEBUG("RAM: Loaded %lu bytes from image file `%s`", nRead, imageFileName);

  return nRead;
}

DataStatus Unibus::Read(un_addr addr) {
  assert((addr & 1) == 0);

  assert(addr <= MEM_UNIBUS_ADDR_MAX);    // TODO: Debug
  addr &= MEM_UNIBUS_ADDR_MAX;

  if(addr < MEM_UNIBUS_PERIPH_PAGE_ADDR)
    return _ram->Read(addr);

  //DEBUG("UNIBUS: I/O read: 0%06o", addr);

  auto io = _getIoHandler(addr);
  if(!io) {
    DEBUG("UNIBUS: Reading unknown periphery page address: 0%08o", addr);
    return DataStatus::Error(MEM_ERR_UNB_TIMEOUT);
  }

  return io->Read(addr);
}

DataStatus Unibus::Write(un_addr addr, const PartialValue& data) {
  assert((addr & 1) == 0);

  assert(addr <= MEM_UNIBUS_ADDR_MAX);    // TODO: Debug
  addr &= MEM_UNIBUS_ADDR_MAX;

  if(addr < MEM_UNIBUS_PERIPH_PAGE_ADDR)
    return _ram->Write(addr, data);

  //DEBUG("UNIBUS: I/O write: 0%06o -> addr 0%06o", data, addr);

  auto io = _getIoHandler(addr);
  if(!io) {
    DEBUG("UNIBUS: Writing unknown periphery page address: 0%08o, data: 0%06o", addr, data.GetValue(0));
    return DataStatus::Error(MEM_ERR_UNB_TIMEOUT);
  }

  io->Write(addr, data);

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
