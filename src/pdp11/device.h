//
// Created by palulukan on 1/9/26.
//

#ifndef DEVICE_H_B697FE97ACF6423BBE8A6F8E433A4E05
#define DEVICE_H_B697FE97ACF6423BBE8A6F8E433A4E05

#include "unibus.h"

#include "../taskScheduler.h"

class Device : public UnibusDevice {
  public:
    Device(const std::shared_ptr<Unibus>& bus, const std::shared_ptr<TaskScheduler> &ts, const std::vector<IoWindow>& ioMap, int irqPriority, const char* name);
    ~Device() override;

  protected:
    void SetIRQ();
    void ClearIRQ();

  protected:
    std::shared_ptr<Unibus> _bus;
    std::shared_ptr<TaskScheduler> _ts;
};

#endif //DEVICE_H_B697FE97ACF6423BBE8A6F8E433A4E05
