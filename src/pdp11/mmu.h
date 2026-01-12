//
// Created by palulukan on 1/11/26.
//

#ifndef MMU_H_A7DC21942F24459FBE40E92721207351
#define MMU_H_A7DC21942F24459FBE40E92721207351

#include "unibus.h"

#include <functional>

#define MMU_MMR0_MMU_EN         (1 << 0)    // Enable Relocation
#define MMU_MMR0_PAGE_NO_MASK   0x000E      // Page Number
#define MMU_MMR0_SET_PAGE_NO(mmr, page) ((mmr) = ((mmr) & ~MMU_MMR0_PAGE_NO_MASK) | (((page) << 1) & MMU_MMR0_PAGE_NO_MASK))
#define MMU_MMR0_PAGE_SPACE     (1 << 4)    // Page Address Space
#define MMU_MMR0_SET_PAGE_SPACE(mmr, space) ((mmr) = ((mmr) & ~MMU_MMR0_PAGE_SPACE) | (((space) >> 4) & 0x0001))
#define MMU_MMR0_CPU_MODE_MASK  0x0060      // Processor Mode
#define MMU_MMR0_GET_CPU_MODE_NO(mmr)       (((mmr) & MMU_MMR0_CPU_MODE_MASK) >> 5)
#define MMU_MMR0_SET_CPU_MODE_NO(mmr, mode) ((mmr) = ((mmr) & ~MMU_MMR0_CPU_MODE_MASK) | (((mode) << 5) & MMU_MMR0_CPU_MODE_MASK))
#define MMU_MMR0_INST_COMPLETED (1 << 7)    // Instruction Completed
#define MMU_MMR0_DST_MODE       (1 << 8)    // Maintenance/Destination Mode
#define MMU_MMR0_TRAP_EN        (1 << 9)    // Enable Memory Management Traps
#define MMU_MMR0_TRAP           (1 << 12)   // Trap - set whenever a Memory Management trap condition occurs
#define MMU_MMR0_ERR_ABRT_RO    (1 << 13)   // Abort - Read Only
#define MMU_MMR0_ERR_ABRT_PL    (1 << 14)   // Abort - Page Length
#define MMU_MMR0_ERR_ABRT_NR    (1 << 15)   // Abort - Non-Resident
#define MMU_MMR0_ERR_MASK   (MMU_MMR0_ERR_ABRT_RO | MMU_MMR0_ERR_ABRT_PL | MMU_MMR0_ERR_ABRT_NR)
#define MMU_MMR0_WR_MASK    (MMU_MMR0_MMU_EN | MMU_MMR0_PAGE_NO_MASK | MMU_MMR0_PAGE_SPACE \
    | MMU_MMR0_CPU_MODE_MASK | MMU_MMR0_DST_MODE | MMU_MMR0_TRAP_EN | MMU_MMR0_TRAP | MMU_MMR0_ERR_MASK)

#define MMU_MMR3_USR_D_EN   (1 << 0)    // Enable User D Space
#define MMU_MMR3_SVR_D_EN   (1 << 1)    // Enable Supervisor D Space
#define MMU_MMR3_KRN_D_EN   (1 << 2)    // Enable Kernel D Space
#define MMU_MMR3_CSM_EN     (1 << 3)    // Enable CSM instruction (11/44 only)
#define MMU_MMR3_22BIT_MAP  (1 << 4)    // Enable 22-bit mapping
#define MMU_MMR3_UB_MAP_REL (1 << 5)    // Enable UNIBUS Map relocation
#define MMU_MMR3_WR_MASK (MMU_MMR3_USR_D_EN | MMU_MMR3_SVR_D_EN | MMU_MMR3_KRN_D_EN | MMU_MMR3_22BIT_MAP | MMU_MMR3_UB_MAP_REL)

#define PDR_ACF_NON_RESIDENT  0   // 000  non-resident    abort all accesses
#define PDR_ACF_RO_TRAP       1   // 001  read-only*      abort on write attempt, memory management trap on read
#define PDR_ACF_RO            2   // 010  read-only       abort on write attempt
#define PDR_ACF_UNUSED_3      3   // 011  unused          abort all accesses-reserved for future use
#define PDR_ACF_RW_TRAP       4   // 100  read/write      memory management trap upon completion of a read or write
#define PDR_ACF_WR_TRAP       5   // 101  read/write*     memory management trap upon completion of a write
#define PDR_ACF_RW            6   // 110  read/write      no system trap/abort action
#define PDR_ACF_UNUSED_7      7   // 111  unused          abort all accesses-reserved for future use

#define PDR_ACF_MASK        0x0007  // Access Control Field (ACF)

#define PDR_GET_ACF(pdr)    ((pdr) & PDR_ACF_MASK)
#define PDR_ED              (1 << 3)    // Expansion Direction
#define PDR_W               (1 << 6)    // Access Information Bit W
#define PDR_A               (1 << 7)    // Access Information Bit A
#define PDR_PLF_MASK        0x7F00      // Page Length Field
#define PDR_GET_PLF(pdr)    (((pdr) >> 8) & 0x7F)
#define PDR_WR_MASK         (PDR_PLF_MASK | PDR_ED | PDR_ACF_MASK)

typedef enum {
  cpu_space_I = 0,
  cpu_space_D = 1,

  _cpu_space_max
} cpu_space;

typedef enum {
  cpu_mode_Kernel = 0,
  cpu_mode_Supervisor = 1,
  cpu_mode_Invalid = 2,
  cpu_mode_User = 3,

  _cpu_mode_max
} cpu_mode;

class MMU : public UnibusDevice {
  public:
    MMU();

    void OnTrap(const std::function<void()>& cbTrap) { _cbTrap = cbTrap; }

    void Reset() override;
    cpu_word Read(un_addr addr) override;
    void Write(un_addr addr, cpu_word data, cpu_word mask) override;
    cpu_word IrqAck() override;

    void UpdateMMR0(bool bInstCompleted);
    void ResetMMR1();
    void UpdateMMR1(int reg, int diff);
    void UpdateMMR2(cpu_word val);

    addr_status_t Map(cpu_addr addr, cpu_space s, cpu_mode m, bool bWR);

    cpu_word Get_MMR0() const { return _MMR[0]; }
    cpu_word Get_MMR1() const { return _MMR[1]; }
    cpu_word Get_MMR2() const { return _MMR[2]; }
    cpu_word Get_MMR3() const { return _MMR[3]; }
    const cpu_word* Get_PAR(int s, int m) const { return _pageRegModes[m].pageRegSpaces[s].PAR; }
    const cpu_word* Get_PDR(int s, int m) const { return _pageRegModes[m].pageRegSpaces[s].PDR; }

  public:
    static addr_status_t Map16Bit(cpu_addr addr);

  private:
    void _mmuAbort(cpu_word &PDR, int pageNo, cpu_space s, cpu_mode m, cpu_word errFlags);
    void _mmuTrap(cpu_word &PDR);
    static std::vector<IoWindow> _buildIoMap();

  private:
    std::function<void()> _cbTrap;

    cpu_word _MMR[4] = {};

    struct {
      struct {
        cpu_word PAR[8];
        cpu_word PDR[8];
      } pageRegSpaces[_cpu_space_max] = {};
    } _pageRegModes[_cpu_mode_max] = {};
};

#endif //MMU_H_A7DC21942F24459FBE40E92721207351
