//
// Created by palulukan on 1/10/26.
//

#include "dl11.h"

#include "../log.h"

#include <cassert>
#include <cstring>

#define DL11_IRQ_PRIORITY 4

DL11::DL11(const std::shared_ptr<Unibus>& bus, const std::shared_ptr<TaskScheduler> &ts, un_addr baseAddr, cpu_word baseVector):
  Device(bus, ts, { { .start = baseAddr, .size = DL11_NREGS * MEM_WORD_SIZE } }, DL11_IRQ_PRIORITY, "DL11"),
  _baseAddr(baseAddr),
  _baseVector(baseVector)
{
  DL11::Reset();
}

DL11::~DL11() {
  _ts->Cancel(_txTask);
  _txTask = TS_NULL_TASK;

  _ts->Cancel(_rxTask);
  _rxTask = TS_NULL_TASK;
}

bool DL11::Receive(char ch) {
  // Ignore receiver input while in maintenance mode
  if(_regs[DL11_XCSR] & DL11_XCSR_MAINT)
    return true;

  return _runReceiver(ch);
}

void DL11::Reset() {
  memset(_regs, 0, sizeof(_regs));

  _regs[DL11_XCSR] = DL11_XCSR_XMIT_RDY;

  _ts->Cancel(_txTask);
  _txTask = TS_NULL_TASK;

  _ts->Cancel(_rxTask);
  _rxTask = TS_NULL_TASK;

  _txPending = false;

  _updateInterrupts();
}

cpu_word DL11::Read(un_addr addr) {
  assert(addr >= _baseAddr);
  assert(addr < _baseAddr + (DL11_NREGS * MEM_WORD_SIZE));
  assert((addr & 1) == 0);

  auto reg = (addr - _baseAddr) >> 1;

  // static const char *regNames[] = { "RCSR", "RBUF", "XCSR", "XBUF" };
  // DEBUG("DL11: Reading %s", regNames[reg]);

  auto res = _regs[reg];

  if(reg == DL11_RCSR) {
    // DL11_RCSR_DATASET_INT is read-once bit so clear it when register is read.
    _regs[DL11_RCSR] &= ~DL11_RCSR_DATASET_INT;

    _updateInterrupts();
  } else if(reg == DL11_RBUF) {
    // DL11_RCSR_RCVR_DONE should be cleared when RBUF register is addressed.
    _regs[DL11_RCSR] &= ~DL11_RCSR_RCVR_DONE;

    _updateInterrupts();
  }

  return res;
}

void DL11::Write(un_addr addr, const PartialValue& data) {
  assert((addr & 1) == 0);
  assert(addr >= _baseAddr);
  assert(addr < _baseAddr + (DL11_NREGS * MEM_WORD_SIZE));

  auto reg = (addr - _baseAddr) >> 1;

  switch(reg) {
    case DL11_RCSR: {
      // DEBUG("DL11: Writing RCSR: 0%06o", data);

      auto v = data.GetValue(_regs[DL11_RCSR]);

      assert(!(v & DL11_RCSR_DATASET_INT_ENB));

      _regs[DL11_RCSR] = (_regs[DL11_RCSR] & ~DL11_RCSR_WR_MASK) | (v & DL11_RCSR_WR_MASK);

      if(v & DL11_RCSR_RDR_ENB)
        _regs[DL11_RCSR] &= ~DL11_RCSR_RCVR_DONE;

      _updateInterrupts();
      break;
    }

    case DL11_RBUF: {
      // DEBUG("DL11: Writing RBUF: 0%06o", data);

      // DL11_RCSR_RCVR_DONE should be cleared when RBUF register is addressed.
      // No actual write is performed since RBUF is read-only
      _regs[DL11_RCSR] &= ~DL11_RCSR_RCVR_DONE;

      _updateInterrupts();
      break;
    }

    case DL11_XCSR: {
      // DEBUG("DL11: Writing XCSR: 0%06o", data);

      auto v = data.GetValue(_regs[DL11_XCSR]);

      _regs[DL11_XCSR] = (_regs[DL11_XCSR] & ~DL11_XCSR_WR_MASK) | (v & DL11_XCSR_WR_MASK);

      if(v & DL11_XCSR_MAINT) {
        _ts->Cancel(_rxTask);
        _rxTask = TS_NULL_TASK;
      }

      _updateInterrupts();
      break;
    }

    case DL11_XBUF: {
      // DEBUG("DL11: Writing XBUF: 0%06o", data);

      DL11_XBUF_SET_DATA(_regs[DL11_XBUF], data.GetValue(_regs[DL11_XBUF]));

      _runTransmitter();
      break;
    }

    default:
      assert(false);
  }
}

cpu_word DL11::IrqAck() {
  ClearIRQ();

  // _baseVector -> RX, higher priority than TX
  // _baseVector + 4 -> TX

  if(_regs[DL11_RCSR] & (DL11_RCSR_DATASET_INT | DL11_RCSR_RCVR_DONE))
    return _baseVector;

  if(_regs[DL11_XCSR] & DL11_XCSR_XMIT_RDY)
    return _baseVector + 4;

  // Should not be here
  assert(false);
}

void DL11::_updateInterrupts() {
  const auto dataset_mask = DL11_RCSR_DATASET_INT_ENB | DL11_RCSR_DATASET_INT;
  const auto rcvr_mask = DL11_RCSR_RCVR_INT_ENB | DL11_RCSR_RCVR_DONE;
  const auto xmit_mask = DL11_XCSR_XMIT_INT_ENB | DL11_XCSR_XMIT_RDY;

  if(((_regs[DL11_RCSR] & dataset_mask) == dataset_mask) || ((_regs[DL11_RCSR] & rcvr_mask) == rcvr_mask) || ((_regs[DL11_XCSR] & xmit_mask) == xmit_mask))
    SetIRQ();
  else
    ClearIRQ();
}

bool DL11::_runReceiver(char ch) {
  if(_rxTask != TS_NULL_TASK)
    return false;

  _regs[DL11_RCSR] |= DL11_RCSR_RCVR_ACT;

  _rxTask = _ts->SetTimeout([this, ch]() {
    _rxTask = TS_NULL_TASK;

    // When new character received all error bits are cleared.
    // We might as well clear data bits too since they will
    // be overwritten shortly.
    _regs[DL11_RBUF] = 0;

    // Set Overflow error flag if previous character was not processed yet.
    // Also make sure to set DL11_RBUF_ERROR because it represents  logical "or"
    // of all other error bits in RBUF register.
    if(_regs[DL11_RCSR] & DL11_RCSR_RCVR_DONE)
      _regs[DL11_RBUF] |= DL11_RBUF_OR_ERR | DL11_RBUF_ERROR;

    DL11_RBUF_SET_DATA(_regs[DL11_RBUF], ch);

    _regs[DL11_RCSR] &= ~DL11_RCSR_RCVR_ACT;
    _regs[DL11_RCSR] |= DL11_RCSR_RCVR_DONE;

    _updateInterrupts();
  }, DL11_XMIT_TIME_US * TS_MICROSECONDS);

  return true;
}

void DL11::_runTransmitter() {
  if(!(_regs[DL11_XCSR] & DL11_XCSR_XMIT_RDY)) {
    _txPending = true;
    return;
  }

  _txPending = false;
  _regs[DL11_XCSR] &= ~DL11_XCSR_XMIT_RDY;

  const char ch = DL11_XBUF_GET_DATA(_regs[DL11_XBUF]);

  _txTask = _ts->SetTimeout([this, ch]() {
    _txTask = TS_NULL_TASK;

    if(!(_regs[DL11_XCSR] & DL11_XCSR_MAINT) && _onTx)
      _onTx(ch);

    _regs[DL11_XCSR] |= DL11_XCSR_XMIT_RDY;
    if(_txPending)
      _runTransmitter();
    else
      _updateInterrupts();
  }, DL11_XMIT_TIME_US * TS_MICROSECONDS);

  if(_regs[DL11_XCSR] & DL11_XCSR_MAINT)
    _runReceiver(ch);

  _updateInterrupts();
}
