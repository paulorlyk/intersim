//
// Created by palulukan on 1/11/26.
//

#ifndef RK11_H_AB5EB1C23E7A4970A44BB9D0674C11C7
#define RK11_H_AB5EB1C23E7A4970A44BB9D0674C11C7

#include "device.h"

// RK11
// Moving head disk drive controller

#define CONFIG_RK11_NO_DELAYS

#define RK11_NREGS 8

#define RK05_DISKS_MAX  8
#define RK05_SIZE_WORDS 1247232

// RKDS - Device Status Register
// Bit  Designation
// 0    Sector counter [0]
// 1    Sector counter [1]
// 2    Sector counter [2]
// 3    Sector counter [3]
// 4    SC=SA
// 5    Write Protect Status (WPS)
// 6    Read/Write/Seek Ready (R/W/S ROY)
// 7    Drive Ready (DRY)
// 8    Sector Counter OK(SOK)
// 9    Seek Incomplete (SIN)
// 10   Drive Unsafe (DRU)
// 11   RK05 Disk on Line (RK05)
// 12   Drive Power Low (DPL)
// 13   Identification of Drive (ID) [0]
// 14   Identification of Drive (ID) [1]
// 15   Identification of Drive (ID) [2]
#define RK11_RKDS                   0
#define RK11_RKDS_GET_SECTOR(rkds)  ((rkds) & 0x000F)
#define RK11_RKDS_SCSA              (1 << 4)
#define RK11_RKDS_WPS               (1 << 5)
#define RK11_RKDS_ROY               (1 << 6)
#define RK11_RKDS_DRY               (1 << 7)
#define RK11_RKDS_SOK               (1 << 8)
#define RK11_RKDS_RK05              (1 << 11)
#define RK11_RKDS_DRIVE_MASK        0xE000
#define RK11_RKDS_SET_DRIVE(rkds, drv)   ((rkds) = ((rkds) & ~RK11_RKDS_DRIVE_MASK) | (((drv) & 7) << 13))

// RKER - Error Register
// Bit  Designation
// 0    Write Check Error (WCE)
// 1    Checksum Error (CSE)
// 2    Unused
// 3    Unused
// 4    Unused
// 5    Nonexistent Sector (NXS)
// 6    Nonexistent Cylinder (NXC)
// 7    Nonexistent Disk (NXD)
// 8    Timing Error (TE)
// 9    Data Late (DLT)
// 10   Nonexistent Memory (NXM)
// 11   Programming Error (PGE)
// 12   Seek Error (SKE)
// 13   Write Lockout Violation (WLO)
// 14   Overrun (OVR)
// 15   Drive Error (DRE)
#define RK11_RKER   1
#define RK11_RKER_NXS (1 << 5)
#define RK11_RKER_NXC (1 << 6)
#define RK11_RKER_NXD (1 << 7)
#define RK11_RKER_NXM (1 << 10)
#define RK11_RKER_PGE (1 << 11)
#define RK11_RKER_WLO (1 << 13)
#define RK11_RKER_OVR (1 << 14)
#define RK11_RKER_DRE (1 << 15)
#define RK11_RKER_HARD_MASK 0xFFE0

// RKCS - Control Status Register
// Bit  Designation
// 0    GO (Write Only)
// 1    Function (Read/Write) [0]
// 2    Function (Read/Write) [1]
// 3    Function (Read/Write) [2]
// 4    Memory Extension (MEX) (Read/Write) [0]
// 5    Memory Extension (MEX) (Read/Write) [1]
// 6    Interrupt on Done Enable (IDE) (Read/Write)
// 7    Control Ready (RDY) (Read Only)
// 8    Stop on Soft Error (SSE) (Read/Write)
// 9    Extra Bit (EXB)
// 10   Format (FMT) (Read/Write)
// 11   Inhibit Incrementing the RKBA (IBA) (Read/Write)
// 12   Unused
// 13   Search Complete (SCP) (Read Only)
// 14   Hard Error (HE)
// 15   Error (ERR) (Read Only)
#define RK11_RKCS   2
#define RK11_RKCS_GO  (1 << 0)
#define RK11_RKCS_IDE (1 << 6)
#define RK11_RKCS_RDY (1 << 7)
#define RK11_RKCS_EXB (1 << 9)
#define RK11_RKCS_FMT (1 << 10)
#define RK11_RKCS_IBA (1 << 11)
#define RK11_RKCS_UN  (1 << 12)
#define RK11_RKCS_SCP (1 << 13)
#define RK11_RKCS_HE  (1 << 14)
#define RK11_RKCS_ERR (1 << 15)

#define RK11_RKCS_GET_MEX(rkcs)         (((rkcs) >> 4) & 3)
#define RK11_RKCS_SET_MEX(rkcs, mex)    ((rkcs) = ((rkcs) & ~(3 << 4)) | (((mex) & 3) << 4))

#define RK11_RKCS_GET_FUNC(rkcs)        (((rkcs) >> 1) & 7)
#define RK11_RKCS_FUNC_CONTROL_RESET    0
#define RK11_RKCS_FUNC_WRITE            1
#define RK11_RKCS_FUNC_READ             2
#define RK11_RKCS_FUNC_WRITE_CHECK      3
#define RK11_RKCS_FUNC_SEEK             4
#define RK11_RKCS_FUNC_READ_CHECK       5
#define RK11_RKCS_FUNC_DRIVE_RESET      6
#define RK11_RKCS_FUNC_WRITE_LOCK       7

#define RK11_RKWC   3

#define RK11_RKBA   4

#define RK11_RKDA   5
#define RK11_RKDA_GET_SECTOR(rkda)          ((rkda) & 0x000F)
#define RK11_RKDA_SET_SECTOR(rkda, sec)     ((rkda) = ((rkda) & ~0x000F) | ((sec) & 0x000F))
#define RK11_RKDA_SECTORS                   12
#define RK11_RKDA_GET_SURFACE(rkda)         (((rkda) >> 4) & 0x0001)
#define RK11_RKDA_SET_SURFACE(rkda, surf)   ((rkda) = ((rkda) & ~(1 << 4)) | (((surf) & 1) << 4))
#define RK11_RKDA_SURFACES                  2
#define RK11_RKDA_GET_CYLINDER(rkda)        (((rkda) >> 5) & 0x00FF)
#define RK11_RKDA_SET_CYLINDER(rkda, cyl)   ((rkda) = ((rkda) & ~(0x00FF << 5)) | (((cyl) & 0x00FF) << 5))
#define RK11_RKDA_CYLINDERS                 0313
#define RK11_RKDA_GET_DRIVE(rkda)           (((rkda) >> 13) & 7)

#define RK11_RKMR   6

#define RK11_RKDB   7

struct RK05 {
  explicit RK05(const std::shared_ptr<TaskScheduler>& ts):
    _ts(ts)
  {}

  ~RK05() {
    Unload();
  }

  void Unload();
  void Seek(unsigned int toCylinder, const std::function<void()>& onCompleted);
  void Cancel();
  bool IsIdle() const { return _task == TS_NULL_TASK; }

  std::vector<uint8_t> img;
  bool connected = false;
  bool irq = false;
  bool writeProtect = false;
  unsigned int cylinder = 0;

  private:
    std::shared_ptr<TaskScheduler> _ts;
    ts_task_id _task = TS_NULL_TASK;
};

class RK11 : public Device {
  public:
    RK11(const std::shared_ptr<Unibus>& bus, const std::shared_ptr<TaskScheduler>& ts);

    void Reset() override;
    cpu_word Read(un_addr addr) override;
    void Write(un_addr addr, cpu_word data, cpu_word mask) override;
    cpu_word IrqAck() override;

    bool ConnectDisk(int nDisk, bool connected);
    bool LoadDisk(const char* imageFileName, int nDisk);

    const RK05& Get_RK05(int n) const { return _disks[n]; };
    cpu_word Get_RKDS() const { return _regs[RK11_RKDS]; };
    cpu_word Get_RKER() const { return _regs[RK11_RKER]; };
    cpu_word Get_RKCS() const { return _regs[RK11_RKCS]; };
    cpu_word Get_RKWC() const { return _regs[RK11_RKWC]; };
    cpu_word Get_RKBA() const { return _regs[RK11_RKBA]; };
    cpu_word Get_RKDA() const { return _regs[RK11_RKDA]; };
    cpu_word Get_RKMR() const { return _regs[RK11_RKMR]; };
    cpu_word Get_RKDB() const { return _regs[RK11_RKDB]; };
    bool Get_IRQ() const { return _irq; };

  private:
    void _controlReset();
    void _updateInterrupts();
    void _unloadDisk(int n);
    void _runFunction();
    void _finishFunction();

  private:
    RK05 _disks[RK05_DISKS_MAX];
    cpu_word _regs[RK11_NREGS] = {};
    bool _irq = false;
    int _currentDrive = 0;
};

#endif //RK11_H_AB5EB1C23E7A4970A44BB9D0674C11C7
