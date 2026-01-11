//
// Created by palulukan on 1/9/26.
//

#include "device.h"

Device::Device(const std::shared_ptr<Unibus> &bus, const std::shared_ptr<TaskScheduler> &ts, const std::vector<IoWindow> &ioMap, int irqPriority, const char* name):
  UnibusDevice(ioMap, irqPriority, name),
  _bus(bus),
  _ts(ts)
{
  _bus->RegisterDevice(this);
}

Device::~Device() {
  _bus->UnregisterDevice(this);
}

void Device::SetIRQ() {
  if(_bIRQ)
    return;

  this->_bIRQ = true;

  _bus->NotifyIRQ(this);
}

void Device::ClearIRQ() {
  if(!_bIRQ)
    return;

  this->_bIRQ = false;

  _bus->NotifyIRQ(this);
}
