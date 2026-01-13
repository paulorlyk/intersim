//
// Created by palulukan on 1/11/26.
//

#include "rk11.h"

#include "../log.h"

#include <cstring>
#include <cassert>
#include <cstdlib>

#define RK11_REG_RKDS   0777400
//#define RK11_REG_RKER   0777402
#define RK11_REG_RKCS   0777404
#define RK11_REG_RKWC   0777406
#define RK11_REG_RKBA   0777410
#define RK11_REG_RKDA   0777412
//#define RK11_REG_RKMR   0777414
#define RK11_REG_RKDB   0777416

#define RK11_PERIPH_START   RK11_REG_RKDS

#define RK11_IRQ            0220
#define RK11_IRQ_PRIORITY   5

#define RK05_SECTORS    12

#define RK11_CYL_SEEK_TIME_US   250

#define RK11_SECTOR_SIZE_WORDS    256

void RK05::Unload() {
  Cancel();

  img.clear();
  irq = false;
  writeProtect = false;
  cylinder = 0;
}

void RK05::Seek(unsigned int toCylinder, const std::function<void()> &onCompleted) {
  assert(IsIdle());

  auto cb = [this, toCylinder, onCompleted]() {
    _task = TS_NULL_TASK;

    cylinder = toCylinder;

    onCompleted();
  };

#ifndef CONFIG_RK11_NO_DELAYS
  auto opTime = RK11_CYL_SEEK_TIME_US * (std::abs((int)toCylinder - (int)cylinder) + 1);

  _task = _ts->SetTimeout(cb, opTime * TS_MICROSECONDS);
#else
  cb();
#endif

}

void RK05::Cancel() {
  _ts->Cancel(_task);
  _task = TS_NULL_TASK;
}

RK11::RK11(const std::shared_ptr<Unibus> &bus, const std::shared_ptr<TaskScheduler> &ts) :
  Device(bus, ts, { { .start = RK11_PERIPH_START, .size = RK11_NREGS * MEM_WORD_SIZE } }, RK11_IRQ_PRIORITY, "RK11"),
  _disks{ RK05(_ts), RK05(_ts), RK05(_ts), RK05(_ts), RK05(_ts), RK05(_ts), RK05(_ts), RK05(_ts) }
{
  RK11::Reset();
}

bool RK11::ConnectDisk(int nDisk, bool connected) {
  if(nDisk < 0 || nDisk >= RK11_DISKS_MAX) {
    DEBUG("RK11: Attempting to connect disk with invalid number %d", nDisk);
    return false;
  }

  if(!connected && _disks[nDisk].connected) {
    _unloadDisk(nDisk);
    _disks[nDisk].connected = false;

    DEBUG("RK11: Disk %d disconnected", nDisk);
  }

  if(connected && !_disks[nDisk].connected) {
    _disks[nDisk].connected = true;

    DEBUG("RK11: Disk %d connected", nDisk);
  }

  return true;
}

bool RK11::LoadDisk(const char *imageFileName, int nDisk) {
  if(!imageFileName) {
    DEBUG("RK11: Need disk image file name");
    return false;
  }

  if(nDisk < 0 || nDisk >= RK11_DISKS_MAX) {
    DEBUG("RK11: Attempting to load disk with invalid number %d", nDisk);
    return false;
  }

  if(!ConnectDisk(nDisk, false) || !ConnectDisk(nDisk, true))
    return false;

  FILE *pfImage = fopen(imageFileName, "rb");
  if(!pfImage) {
    DEBUG("RK11: Failed to open image file `%s`", imageFileName);
    return false;
  }

  const size_t diskSize = (size_t)RK05_DISK_SIZE_WORDS * MEM_WORD_SIZE;

  _disks[nDisk].img.resize(diskSize);

  size_t nRead = 0;
  while(nRead < diskSize) {
    nRead += fread(_disks[nDisk].img.data() + nRead, 1, diskSize - nRead, pfImage);

    if(feof(pfImage))
      break;

    if(ferror(pfImage)) {
      DEBUG("RK11: Error reading the disk image file `%s`", imageFileName);

      fclose(pfImage);
      ConnectDisk(nDisk, false);
      return false;
    }
  }

  fclose(pfImage);

  DEBUG("RK11: Loaded %lu bytes from image file `%s` to disk %d", nRead, imageFileName, nDisk);

  return true;
}

void RK11::Reset() {
  for(auto &disk : _disks)
    disk.writeProtect = false;

  _controlReset();
}

cpu_word RK11::Read(un_addr addr) {
  assert(addr >= RK11_PERIPH_START);
  assert(addr < RK11_PERIPH_START + (RK11_NREGS * MEM_WORD_SIZE));
  assert((addr & 1) == 0);

  auto reg = (addr - RK11_PERIPH_START) >> 1;

  switch(reg) {
    case RK11_RKDS: {
      auto &disk = _disks[RK11_RKDA_GET_DRIVE(_regs[RK11_RKDS])];

      _regs[RK11_RKDS] &= RK11_RKDS_DRIVE_MASK;
      if(disk.connected) {
        // "Rotate" disk to the random sector if it is loaded
        if(!disk.img.empty())
          _regs[RK11_RKDS] |= rand() % RK05_SECTORS;

        _regs[RK11_RKDS] |= RK11_RKDS_RK05
                            | RK11_RKDS_SOK
                            | (disk.img.empty() ? 0 : RK11_RKDS_DRY)
                            | (disk.IsIdle() ? RK11_RKDS_ROY : 0)
                            | (disk.writeProtect ? RK11_RKDS_WPS : 0)
                            | (RK11_RKDS_GET_SECTOR(_regs[RK11_RKDS]) == RK11_RKDA_GET_SECTOR(_regs[RK11_RKDA]) ? RK11_RKDS_SCSA : 0);
      }

      break;
    }

    // case RK11_RKCS: {
    //   _regs[RK11_RKCS] &= ~(RK11_RKCS_GO | RK11_RKCS_EXB | RK11_RKCS_UN | RK11_RKCS_HE | RK11_RKCS_ERR);
    //   _regs[RK11_RKCS] |= ((_regs[RK11_RKER] & RK11_RKER_HARD_MASK) ? RK11_RKCS_HE : 0) | ((_regs[RK11_RKER] & ~RK11_RKER_HARD_MASK) ? RK11_RKCS_ERR : 0);
    //
    //   break;
    // }

    default:
      break;
  }

  return _regs[reg];
}

void RK11::Write(un_addr addr, const PartialValue& data) {
  assert((addr & 1) == 0);
  assert(addr >= RK11_PERIPH_START);
  assert(addr < RK11_PERIPH_START + (RK11_NREGS * MEM_WORD_SIZE));

  switch(addr) {
    case RK11_REG_RKCS: {
      //DEBUG("RK11: Writing RKCS: 0%06o", data);

      const bool bIDE = (_regs[RK11_RKCS] & RK11_RKCS_IDE) != 0;

      auto v = data.GetValue(_regs[RK11_RKCS]);

      // Preserve read-only bits
      const cpu_word roMask =
            RK11_RKCS_GO
          | RK11_RKCS_RDY
          | RK11_RKCS_EXB
          | RK11_RKCS_SCP
          | RK11_RKCS_UN
          | RK11_RKCS_HE
          | RK11_RKCS_ERR;
      _regs[RK11_RKCS] &= roMask;
      _regs[RK11_RKCS] |= v & ~roMask;

      if(bIDE != ((v & RK11_RKCS_IDE) != 0)) {
        // TODO: Clear all pending interrupt flags or just disable interrupt delivery?
        _updateInterrupts();
      }

      if((_regs[RK11_RKCS] & RK11_RKCS_RDY) && (v & RK11_RKCS_GO))
        _runFunction();

      break;
    }

    case RK11_REG_RKWC: {
      //DEBUG("RK11: Writing RKWC: 0%06o", data);

      _regs[RK11_RKWC] = data.GetValue(_regs[RK11_RKWC]);
      break;
    }

    case RK11_REG_RKBA: {
      //DEBUG("RK11: Writing RKBA: 0%06o", data);

      _regs[RK11_RKBA] = data.GetValue(_regs[RK11_RKBA]) & 0xFFFE;
      break;
    }

    case RK11_REG_RKDA: {
      //DEBUG("RK11: Writing RKDA: 0%06o", data);

      if(_regs[RK11_RKCS] & RK11_RKCS_RDY)
        _regs[RK11_RKDA] = data.GetValue(_regs[RK11_RKDA]);

      break;
    }

    default: {
      assert(false);
      break;
    }
  }
}

cpu_word RK11::IrqAck() {
  if(!_irq) {
    for(int i = 0; i < RK11_DISKS_MAX; ++i) {
      if(_disks[i].irq) {
        RK11_RKDS_SET_DRIVE(_regs[RK11_RKDS], i);
        _disks[i].irq = false;

        break;
      }
    }
  } else {
    RK11_RKDS_SET_DRIVE(_regs[RK11_RKDS], _currentDrive);
    _irq = false;
  }

  _updateInterrupts();

  return RK11_IRQ;
}

void RK11::_controlReset() {
  // TODO: Is this needed?
  // _bINT = false;

  for(auto &disk : _disks) {
    disk.Cancel();

    disk.irq = false;
  }

  memset(_regs, 0, sizeof(_regs));
  _regs[RK11_RKCS] = RK11_RKCS_RDY;

  _updateInterrupts();
}

void RK11::_updateInterrupts() {
  // Interrupts disabled
  if(!(_regs[RK11_RKCS] & RK11_RKCS_IDE)) {
    _irq = false;

    for(auto &disk : _disks)
      disk.irq = false;

    ClearIRQ();

    return;
  }

  if(!(_regs[RK11_RKCS] & RK11_RKCS_RDY))
    return;

  if(_irq) {
    SetIRQ();
    return;
  }

  for(auto &disk : _disks) {
    if(disk.irq) {
      SetIRQ();
      return;
    }
  }

  ClearIRQ();
}

void RK11::_unloadDisk(int n) {
  assert(n >=0 && n < RK11_DISKS_MAX);

  _disks[n].Unload();

  _updateInterrupts();
}

void RK11::_runFunction() {
  // Must be cleared at the initiation of any new function
  _regs[RK11_RKCS] &= ~(RK11_RKCS_SCP | RK11_RKCS_RDY);
  _regs[RK11_RKER] &= RK11_RKER_HARD_MASK;  // Clear all soft errors

  // _bINT = false;
  // _updateInterrupts();

  _currentDrive = RK11_RKDA_GET_DRIVE(_regs[RK11_RKDA]);

  int func = RK11_RKCS_GET_FUNC(_regs[RK11_RKCS]);

  // Control Reset should always work
  if(func == RK11_RKCS_FUNC_CONTROL_RESET) {
    _controlReset();

    _finishFunction();
    return;
  }

  auto &disk = _disks[_currentDrive];
  if(!disk.connected) {
    _regs[RK11_RKER] |= RK11_RKER_NXD;

    _finishFunction();
    return;
  }

  if(disk.img.empty() || !disk.IsIdle()) {
    _regs[RK11_RKER] |= RK11_RKER_DRE;

    _finishFunction();
    return;
  }

  if((_regs[RK11_RKCS] & RK11_RKCS_FMT) && func != RK11_RKCS_FUNC_READ && func != RK11_RKCS_FUNC_WRITE) {
    _regs[RK11_RKER] |= RK11_RKER_PGE;

    _finishFunction();
    return;
  }

  // TODO: Implement format
  if(_regs[RK11_RKCS] & RK11_RKCS_FMT) {
    DEBUG("RK11: Format bit is set in RKCS");
    assert(false);
  }

  if(func == RK11_RKCS_FUNC_WRITE_LOCK) {
    disk.writeProtect = true;

    _finishFunction();
    return;
  }

  int nSector = 0;
  int nSurface = 0;
  int nCylinder = 0;
  if(func != RK11_RKCS_FUNC_DRIVE_RESET) {
    nSector = RK11_RKDA_GET_SECTOR(_regs[RK11_RKDA]);
    if(nSector >= RK11_RKDA_SECTORS) {
      _regs[RK11_RKER] |= RK11_RKER_NXS;

      _finishFunction();
      return;
    }

    nSurface = RK11_RKDA_GET_SURFACE(_regs[RK11_RKDA]);

    nCylinder = RK11_RKDA_GET_CYLINDER(_regs[RK11_RKDA]);
    if(nCylinder >= RK11_RKDA_CYLINDERS) {
      _regs[RK11_RKER] |= RK11_RKER_NXC;

      _finishFunction();
      return;
    }
  } else {
    disk.writeProtect = false;
    func = RK11_RKCS_FUNC_SEEK;
  }

  if(func == RK11_RKCS_FUNC_WRITE && disk.writeProtect) {
    _regs[RK11_RKER] |= RK11_RKER_WLO;

    _finishFunction();
    return;
  }

  disk.Seek(nCylinder, [this, &disk, func, nSurface, nSector]() {
    if(func == RK11_RKCS_FUNC_SEEK) {
      _regs[RK11_RKCS] |= RK11_RKCS_SCP;

      disk.irq = true;
      _updateInterrupts();
      return;
    }

    // Is the drive still connected and loaded?
    if(!disk.connected || disk.img.empty()) {
      _regs[RK11_RKER] |= RK11_RKER_DRE;

      _finishFunction();
      return;
    }

    unsigned int nDiskWord = ((((disk.cylinder * RK11_RKDA_SURFACES) + nSurface) * RK11_RKDA_SECTORS) + nSector) * RK11_SECTOR_SIZE_WORDS;

    // Use direct value of RKWC here because apparently it is allowed
    // to change words count of the function while it is running
    unsigned int nWordsCount = 0x10000 - _regs[RK11_RKWC];

    if((nDiskWord + nWordsCount) > RK05_DISK_SIZE_WORDS) {
      nWordsCount = RK05_DISK_SIZE_WORDS - nDiskWord;
      _regs[RK11_RKER] |= RK11_RKER_OVR;
    }

    un_addr addr = _regs[RK11_RKBA] | (RK11_RKCS_GET_MEX(_regs[RK11_RKCS]) << 16);
    cpu_word *data = ((cpu_word *)disk.img.data()) + nDiskWord;
    size_t nWordsDone = 0;

    switch(func) {
      case RK11_RKCS_FUNC_WRITE: {
        // TODO: Implement format
        if(_regs[RK11_RKCS] & RK11_RKCS_FMT) {
          DEBUG("RK11: Format bit is set in RKCS");
          assert(false);
        }

        // DEBUG("RK11: Writing %u words to disk %d, cyl %d surf %d sect %d [img 0x%X] from memory location 0%06o",
        //       nWordsCount, _currentDrive, disk.nCylinder, nSurface, nSector, nDiskWord * 2, addr);
        // DEBUG("RK11: Writing %u words to disk [img 0x%06X] from memory location 0%06o", nWordsCount, nDiskWord * 2, addr);

        for(; nWordsDone < nWordsCount; ++nWordsDone) {
          auto w = _bus->Read(addr);
          if(w.error) {
            _regs[RK11_RKER] |= RK11_RKER_NXM;
            break;
          }

          *data++ = w.data;

          if(!(_regs[RK11_RKCS] & RK11_RKCS_IBA))
            addr += MEM_WORD_SIZE;

          // TODO: Write back to the image file
        }

        break;
      }

      case RK11_RKCS_FUNC_READ: {
        // TODO: Implement format
        if(_regs[RK11_RKCS] & RK11_RKCS_FMT) {
          DEBUG("RK11: Format bit is set in RKCS");
          assert(false);
        }

        // DEBUG("RK11: Reading %u words from disk %d, cyl %d surf %d sect %d [img 0x%X] to memory location 0%06o",
        //       nWordsCount, _currentDrive, disk.nCylinder, nSurface, nSector, nDiskWord * 2, addr);
        // DEBUG("RK11: Reading %u words from disk [img 0x%06X] to memory location 0%06o", nWordsCount, nDiskWord * 2, addr);

        for(; nWordsDone < nWordsCount; ++nWordsDone) {
          if(_bus->Write(addr, PartialValue(*data++)).error) {
            _regs[RK11_RKER] |= RK11_RKER_NXM;
            break;
          }

          if(!(_regs[RK11_RKCS] & RK11_RKCS_IBA))
            addr += MEM_WORD_SIZE;
        }

        break;
      }

      case RK11_RKCS_FUNC_WRITE_CHECK: {
        // TODO: Not implemented
        assert(false);
        break;
      }

      case RK11_RKCS_FUNC_READ_CHECK: {
        // TODO: Not implemented
        assert(false);
        break;
      }

      default:
        break;
    }

    nDiskWord = (nDiskWord + nWordsDone) / RK11_SECTOR_SIZE_WORDS;

    auto endSector = nDiskWord % RK11_RKDA_SECTORS;
    auto endSurface = (nDiskWord / RK11_RKDA_SECTORS) & 1;
    auto endCylinder = (nDiskWord / (RK11_RKDA_SECTORS << 1));

    disk.Seek(endCylinder, [this, nWordsDone, addr, endSector, endSurface, endCylinder]() {
      _regs[RK11_RKWC] += nWordsDone;

      _regs[RK11_RKBA] = addr & 0xFFFF;
      RK11_RKCS_SET_MEX(_regs[RK11_RKCS], addr >> 16);

      RK11_RKDA_SET_SECTOR(_regs[RK11_RKDA], endSector);
      RK11_RKDA_SET_SURFACE(_regs[RK11_RKDA], endSurface);
      RK11_RKDA_SET_CYLINDER(_regs[RK11_RKDA], endCylinder);

      _finishFunction();
    });
  });

  if(func == RK11_RKCS_FUNC_SEEK)
    _finishFunction();
}

void RK11::_finishFunction() {
  _regs[RK11_RKCS] |= RK11_RKCS_RDY;
  _irq = true;

  _regs[RK11_RKCS] &= ~(RK11_RKCS_GO | RK11_RKCS_EXB | RK11_RKCS_UN | RK11_RKCS_HE | RK11_RKCS_ERR);
  _regs[RK11_RKCS] |=
      ((_regs[RK11_RKER] & RK11_RKER_HARD_MASK) ? RK11_RKCS_HE : 0)
      | ((_regs[RK11_RKER] & ~RK11_RKER_HARD_MASK) ? RK11_RKCS_ERR : 0);

  _updateInterrupts();
}
