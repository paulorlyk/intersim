//
// Created by palulukan on 1/10/26.
//

#ifndef DL11_H_EBF3E27A830B4A12B74A93833EAD70FD
#define DL11_H_EBF3E27A830B4A12B74A93833EAD70FD

#include "device.h"

#include "../taskScheduler.h"

#include <memory>

// DL11
// Asynchronous line interface

#define DL11_XMIT_TIME_US 1000

#define DL11_NREGS 4

// RCSR - Receiver Status Register
// Bit  Designation
// 0    RDR ENB (Reader Enable)
// 1    DTR (Data Terminal Ready)
// 2    REQ TO SEND (Request to Send)
// 3    SEC XMIT (Secondary Transmit or Supervisory Transmitted Data)
// 4    Unused
// 5    DATASET INT ENB (Dataset Interrupt Enable)
// 6    RCVR INT ENB (Receiver Interrupt Enable)
// 7    RCVR DONE (Receiver Done)
// 8    Unused
// 9    Unused
// 10   SEC REC (Secondary Receive or Supervisory Received Data)
// 11   RCVR_ACT (Receiver Active)
// 12   CAR DET (Carrier Detect)
// 13   CLR TO SEND (Clear to Send)
// 14   RING
// 14   DATASET INT (Dataset Interrupt)
#define DL11_RCSR                   0
#define DL11_RCSR_RDR_ENB           (1 << 0)
#define DL11_RCSR_DTR               (1 << 1)
#define DL11_RCSR_REQ_TO_SEND       (1 << 2)
#define DL11_RCSR_SEC_XMIT          (1 << 3)
#define DL11_RCSR_DATASET_INT_ENB   (1 << 5)
#define DL11_RCSR_RCVR_INT_ENB      (1 << 6)
#define DL11_RCSR_RCVR_DONE         (1 << 7)
#define DL11_RCSR_SEC_REC           (1 << 10)
#define DL11_RCSR_RCVR_ACT          (1 << 11)
#define DL11_RCSR_CAR_DET           (1 << 12)
#define DL11_RCSR_CLR_TO_SEND       (1 << 13)
#define DL11_RCSR_RING              (1 << 14)
#define DL11_RCSR_DATASET_INT       (1 << 15)
#define DL11_RCSR_WR_MASK           (DL11_RCSR_DTR | DL11_RCSR_REQ_TO_SEND | DL11_RCSR_SEC_XMIT | DL11_RCSR_DATASET_INT_ENB | DL11_RCSR_RCVR_INT_ENB)

// RBUF - Receiver Buffer Register
// Bit      Designation
// 0 - 7    RECEIVED DATA BITS
// 8 - 11   Unused
// 12        P ERR (Parity Error)
// 13        FR ERR (Framing Error)
// 14        OR ERR (Overrun Error)
// 15        ERROR
#define DL11_RBUF                       1
#define DL11_RBUF_SET_DATA(rbuf, data)  ((rbuf) = ((rbuf) & ~0xFF) | ((data) & 0xFF))
#define DL11_RBUF_GET_DATA(rbuf)        ((rbuf) & 0xFF)
#define DL11_RBUF_P_ERR                 (1 << 12)
#define DL11_RBUF_FR_ERR                (1 << 13)
#define DL11_RBUF_OR_ERR                (1 << 14)
#define DL11_RBUF_ERROR                 (1 << 15)

// XCSR - Transmitter Status Register
// Bit  Designation
// 0    BREAK
// 1    Unused
// 2    MAINT (Maintenance)
// 3    Unused
// 4    Unused
// 5    Unused
// 6    XMIT INT ENB (Transmitter Interrupt Enable)
// 7    XMIT RDY (Transmitter Ready)
#define DL11_XCSR               2
#define DL11_XCSR_BREAK         (1 << 0)
#define DL11_XCSR_MAINT         (1 << 2)
#define DL11_XCSR_XMIT_INT_ENB  (1 << 6)
#define DL11_XCSR_XMIT_RDY      (1 << 7)
#define DL11_XCSR_WR_MASK       (DL11_XCSR_BREAK | DL11_XCSR_MAINT | DL11_XCSR_XMIT_INT_ENB)

// XBUF - Transmitter Buffer Register
// Bit      Designation
// 0 - 7    TRANSMITTER DATA BUFFER
// 8 - 15   Unused
#define DL11_XBUF                       3
#define DL11_XBUF_SET_DATA(xbuf, data)  ((xbuf) = ((xbuf) & ~0xFF) | ((data) & 0xFF))
#define DL11_XBUF_GET_DATA(xbuf)        ((xbuf) & 0xFF)

class DL11 : public Device {
  public:
    DL11(const std::shared_ptr<Unibus>& bus, const std::shared_ptr<TaskScheduler>& ts, un_addr baseAddr, cpu_word baseVector);
    ~DL11() override;

    void SetOnTx(const std::function<void(char ch)>& onTx) { _onTx = onTx; };
    bool Receive(char ch);

    un_addr GetBaseAddress() const { return _baseAddr; }
    un_addr GetBaseVector() const { return _baseVector; }
    cpu_word Get_RCSR() const { return _regs[DL11_RCSR]; }
    cpu_word Get_RBUF() const { return _regs[DL11_RBUF]; }
    cpu_word Get_XCSR() const { return _regs[DL11_XCSR]; }
    cpu_word Get_XBUF() const { return _regs[DL11_XBUF]; }

  protected:
    void Reset() override;
    cpu_word Read(un_addr addr) override;
    void Write(un_addr addr, const PartialValue& data) override;
    cpu_word IrqAck() override;

  private:
    void _updateInterrupts();
    bool _runReceiver(char ch);
    void _runTransmitter();

  private:
    un_addr _baseAddr;
    cpu_word _baseVector;
    cpu_word _regs[DL11_NREGS] = {};
    ts_task_id _txTask = TS_NULL_TASK;
    ts_task_id _rxTask = TS_NULL_TASK;
    bool _txPending = false;
    std::function<void(char ch)> _onTx;
};

#endif //DL11_H_EBF3E27A830B4A12B74A93833EAD70FD
