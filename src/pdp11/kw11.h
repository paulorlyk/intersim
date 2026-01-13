//
// Created by palulukan on 1/9/26.
//

#ifndef KW11_H_4F8236987C6D44038B4D7FF068946505
#define KW11_H_4F8236987C6D44038B4D7FF068946505

#include "device.h"

// KW11-L
// Line time clock

#define CONFIG_KW11_HZ 60

#define KW11_SR_IE      (1 << 6)    // Interrupt enable
#define KW11_SR_IM      (1 << 7)    // Interrupt monitor
#define KW11_SR_MASK    (KW11_SR_IE | KW11_SR_IM)

class KW11 : public Device {
  public:
    KW11(const std::shared_ptr<Unibus>& bus, const std::shared_ptr<TaskScheduler>& ts);
    ~KW11() override;

    cpu_word Get_SR() const { return _SR; }

  protected:
    void Reset() override;
    cpu_word Read(un_addr addr) override;
    void Write(un_addr addr, const PartialValue& data) override;
    cpu_word IrqAck() override;

  private:
    cpu_word _SR = 0;
    ts_task_id _timer = TS_NULL_TASK;
};

#endif //KW11_H_4F8236987C6D44038B4D7FF068946505
