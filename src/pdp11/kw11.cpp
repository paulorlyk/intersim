//
// Created by palulukan on 1/9/26.
//

#include "kw11.h"

#include <cassert>

#define KW11_ADDR 0777546

#define KW11_IRQ 0100

#define KW11_IRQ_PRIORITY 6

KW11::KW11(const std::shared_ptr<Unibus>& bus, const std::shared_ptr<TaskScheduler> &ts):
  Device(bus, ts, { { .start = KW11_ADDR, .size = MEM_WORD_SIZE } }, KW11_IRQ_PRIORITY, "KW11-L")
{
  KW11::Reset();

#ifdef CONFIG_KW11_HZ
  _timer = _ts->SetInterval([this]() {
    _SR |= KW11_SR_IM;

    if(_SR & KW11_SR_IE)
      SetIRQ();
  }, TS_SECONDS / CONFIG_KW11_HZ);
#endif
}

KW11::~KW11() {
  _ts->Cancel(_timer);
  _timer = TS_NULL_TASK;
}

void KW11::Reset() {
  _SR = KW11_SR_IM;

  ClearIRQ();
}

cpu_word KW11::Read(un_addr addr) {
  assert(addr == KW11_ADDR);
  assert((addr & 1) == 0);

  return _SR;
}

void KW11::Write(un_addr addr, cpu_word data, cpu_word mask) {
  assert((addr & 1) == 0);
  assert(addr == KW11_ADDR);
  assert(mask == 0xFFFF);

  _SR = data & KW11_SR_MASK;

  if(!(_SR & KW11_SR_IE))
    ClearIRQ();
}

cpu_word KW11::IrqAck() {
  ClearIRQ();

  return KW11_IRQ;
}
