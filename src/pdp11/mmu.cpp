//
// Created by palulukan on 1/11/26.
//

#include "mmu.h"

#include "../log.h"

#include <cstring>
#include <cassert>

#define MMU_REG_MMR0  0777572
#define MMU_REG_MMR1  0777574
#define MMU_REG_MMR2  0777576
#define MMU_REG_MMR3  0772516

#define MMU_REG_LOWER_SIZE  0777760
#define MMU_REG_UPPER_SIZE  0777762

struct _mmuRegGroupInfo {
  un_addr start;
  un_addr size;
  cpu_mode mode;
  cpu_space space;
};

static const _mmuRegGroupInfo _mmuGegGroupsTablePDR[6] = {
  { .start = 0777600, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_User,       .space = cpu_space_I }, // UISDR[0-7]
  { .start = 0777620, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_User,       .space = cpu_space_D }, // UDSDR[0-7]

  { .start = 0772200, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_Supervisor, .space = cpu_space_I }, // SISDR[0-7]
  { .start = 0772220, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_Supervisor, .space = cpu_space_D }, // SDSDR[0-7]

  { .start = 0772300, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_Kernel,     .space = cpu_space_I }, // KISDR[0-7]
  { .start = 0772320, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_Kernel,     .space = cpu_space_D }, // KDSDR[0-7]
};

static const _mmuRegGroupInfo _mmuGegGroupsTablePAR[6] = {
  { .start = 0777640, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_User,       .space = cpu_space_I }, // UISAR[0-7]
  { .start = 0777660, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_User,       .space = cpu_space_D }, // UDSAR[0-7]

  { .start = 0772240, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_Supervisor, .space = cpu_space_I }, // SISAR[0-7]
  { .start = 0772260, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_Supervisor, .space = cpu_space_D }, // SDSAR[0-7]

  { .start = 0772340, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_Kernel,     .space = cpu_space_I }, // KISAR[0-7]
  { .start = 0772360, .size = 8 * MEM_WORD_SIZE, .mode = cpu_mode_Kernel,     .space = cpu_space_D }, // KDSAR[0-7]
};

MMU::MMU():
  UnibusDevice(_buildIoMap(), 0, "PDP-11/70 MMU")
{
  MMU::Reset();
}

void MMU::Reset() {
  memset(_MMR, 0, sizeof(_MMR));
  memset(_pageRegModes, 0, sizeof(_pageRegModes));

  _MMR[0] |= MMU_MMR0_INST_COMPLETED | MMU_MMR0_TRAP_EN;
}

cpu_word MMU::Read(un_addr addr) {
  assert(!(addr & 1));

  for(auto &rg : _mmuGegGroupsTablePDR) {
    if(addr >= rg.start && addr < (rg.start + rg.size)) {
      auto nReg = (addr - rg.start) >> 1;

      // DEBUG("MMU: RD PDR%d", nReg);

      return _pageRegModes[rg.mode].pageRegSpaces[rg.space].PDR[nReg];
    }
  }

  for(auto &rg : _mmuGegGroupsTablePAR) {
    if(addr >= rg.start && addr < (rg.start + rg.size)) {
      auto nReg = (addr - rg.start) >> 1;

      // DEBUG("MMU: RD PAR%d", nReg);

      return _pageRegModes[rg.mode].pageRegSpaces[rg.space].PAR[nReg];
    }
  }

  switch(addr) {
    case MMU_REG_MMR0:
      // DEBUG("MMU: RD MMR0");
      return _MMR[0];

    case MMU_REG_MMR1:
      // DEBUG("MMU: RD MMR1");
      return _MMR[1];

    case MMU_REG_MMR2:
      // DEBUG("MMU: RD MMR2");
      return _MMR[2];

    case MMU_REG_MMR3:
      // DEBUG("MMU: RD MMR3");
      return _MMR[3];

    case MMU_REG_LOWER_SIZE:
      return (MEM_SIZE_BYTES >> 6) - 1;

    case MMU_REG_UPPER_SIZE:
      return 0;

    default:
      assert(false);
      break;
  }
}

void MMU::Write(un_addr addr, cpu_word data, cpu_word mask) {
  assert((addr & 1) == 0);
  assert(mask == 0xFFFF);

  for(auto &rg : _mmuGegGroupsTablePDR) {
    if(addr >= rg.start && addr < (rg.start + rg.size)) {
      auto nReg = (addr - rg.start) >> 1;

      // DEBUG("MMU: WR PDR%d", nReg);

      auto &PDR = _pageRegModes[rg.mode].pageRegSpaces[rg.space].PDR[nReg];
      PDR = (PDR & ~PDR_WR_MASK) | (data & PDR_WR_MASK);

      return;
    }
  }

  for(auto &rg : _mmuGegGroupsTablePAR) {
    if(addr >= rg.start && addr < (rg.start + rg.size)) {
      auto nReg = (addr - rg.start) >> 1;

      // DEBUG("MMU: WR PAR%d", nReg);

      _pageRegModes[rg.mode].pageRegSpaces[rg.space].PAR[nReg] = data;
      _pageRegModes[rg.mode].pageRegSpaces[rg.space].PDR[nReg] &= ~(PDR_A | PDR_W);

      return;
    }
  }

  switch(addr) {
    case MMU_REG_MMR0: {
      // DEBUG("MMU: WR MMR0:\n\tEN=%d PAGENO=%d SPACE=%s MODE=%d INSTCOMPLEATED=%d DSTMODE=%d TRAPEN=%d TRAP=%d ABRTRO=%d ABRTPL=%d ABRTNR=%d ->\n\tEN=%d PAGENO=%d SPACE=%s MODE=%d INSTCOMPLEATED=%d DSTMODE=%d TRAPEN=%d TRAP=%d ABRTRO=%d ABRTPL=%d ABRTNR=%d",
      //       _mmr[0] & MMU_MMR0_MMU_EN ? 1 : 0,
      //       (_mmr[0] & MMU_MMR0_PAGE_NO_MASK) >> 1,
      //       _mmr[0] & MMU_MMR0_PAGE_SPACE ? "D" : "I",
      //       MMU_MMR0_GET_CPU_MODE_NO(_mmr[0]),
      //       _mmr[0] & MMU_MMR0_INST_COMPLETED ? 1 : 0,
      //       _mmr[0] & MMU_MMR0_DST_MODE ? 1 : 0,
      //       _mmr[0] & MMU_MMR0_TRAP_EN ? 1 : 0,
      //       _mmr[0] & MMU_MMR0_TRAP ? 1 : 0,
      //       _mmr[0] & MMU_MMR0_ERR_ABRT_RO ? 1 : 0,
      //       _mmr[0] & MMU_MMR0_ERR_ABRT_PL ? 1 : 0,
      //       _mmr[0] & MMU_MMR0_ERR_ABRT_NR ? 1 : 0,
      //       data & MMU_MMR0_MMU_EN ? 1 : 0,
      //       (data & MMU_MMR0_PAGE_NO_MASK) >> 1,
      //       data & MMU_MMR0_PAGE_SPACE ? "D" : "I",
      //       MMU_MMR0_GET_CPU_MODE_NO(data),
      //       data & MMU_MMR0_INST_COMPLETED ? 1 : 0,
      //       data & MMU_MMR0_DST_MODE ? 1 : 0,
      //       data & MMU_MMR0_TRAP_EN ? 1 : 0,
      //       data & MMU_MMR0_TRAP ? 1 : 0,
      //       data & MMU_MMR0_ERR_ABRT_RO ? 1 : 0,
      //       data & MMU_MMR0_ERR_ABRT_PL ? 1 : 0,
      //       data & MMU_MMR0_ERR_ABRT_NR ? 1 : 0
      //     );

      assert(!(data & MMU_MMR0_DST_MODE)); // TODO: Add support for maintenance mode

      _MMR[0] = (_MMR[0] & ~MMU_MMR0_WR_MASK) | (data & MMU_MMR0_WR_MASK);
      break;
    }

    case MMU_REG_MMR1: {
      // DEBUG("MMU: WR MMR1");

      _MMR[1] = data;
      break;
    }

    case MMU_REG_MMR3: {
      DEBUG("MMU: WR MMR3");

      assert(!(data & MMU_MMR3_22BIT_MAP));   // TODO: Implement
      assert(!(data & MMU_MMR3_UB_MAP_REL));  // TODO: Implement

      _MMR[3] = (_MMR[3] & ~MMU_MMR3_WR_MASK) | (data & MMU_MMR3_WR_MASK);
      break;
    }

    case MMU_REG_MMR2:
    case MMU_REG_LOWER_SIZE:
    case MMU_REG_UPPER_SIZE:
      break;

    default: {
      assert(false);
      break;
    }
  }
}

cpu_word MMU::IrqAck() {
  return 0;
}

void MMU::UpdateMMR0(bool bInstCompleted) {
  _MMR[0] = (_MMR[0] & ~MMU_MMR0_INST_COMPLETED) | (bInstCompleted ? MMU_MMR0_INST_COMPLETED : 0);
}

void MMU::ResetMMR1() {
  if(!(_MMR[0] & MMU_MMR0_ERR_MASK))
    _MMR[1] = 0;
}

void MMU::UpdateMMR1(int reg, int diff) {
  assert(reg >= 0);
  assert(reg < 8);
  assert(diff >= -16);
  assert(diff < 16);

  if(!(_MMR[0] & MMU_MMR0_ERR_MASK)) {
    _MMR[1] <<= 8;
    _MMR[1] = ((diff << 3) & 0x00F8) | (reg & 7);
  }
}

void MMU::UpdateMMR2(cpu_word val) {
  if(!(_MMR[0] & MMU_MMR0_ERR_MASK))
    _MMR[2] = val;
}

addr_status_t MMU::Map(cpu_addr addr, cpu_space s, cpu_mode m, bool bWR) {
  assert(s < _cpu_space_max);
  assert(m < _cpu_mode_max);

  if(!(_MMR[0] & MMU_MMR0_MMU_EN)) {
    // Memory Management Unit is inoperative and addresses are not
    // relocated or protected.
    return Map16Bit(addr);
  }

  if(   (m == cpu_mode_Kernel     && !(_MMR[3] & MMU_MMR3_KRN_D_EN))
     || (m == cpu_mode_Supervisor && !(_MMR[3] & MMU_MMR3_SVR_D_EN))
     || (m == cpu_mode_User       && !(_MMR[3] & MMU_MMR3_USR_D_EN))) {
      // When D space is disabled, all references use the I space registers
      s = cpu_space_I;
  }

  auto pageNo = (addr >> 13) & 7;

  // Select Page Address Register (PAR) and Page Descriptor Register (PDR).
  // Each CPU mode (Kernel, Supervisor and User) has it's own set
  // of PARs and PDRs for each space (Instruction and Data).
  // PAR for current space and mode is selected by
  // Active Page Field (APF) which is 3 most significant
  // bits of the virtual address. The rest 13 bits of the Virtual
  // Address (VA) are called Displacement Field (DF).
  // It consists of Block Number (BN) - higher 7 bits and
  // Displacement In Block (DIB) - lower 6 bits.
  auto PAR = _pageRegModes[m].pageRegSpaces[s].PAR[pageNo];
  auto &PDR = _pageRegModes[m].pageRegSpaces[s].PDR[pageNo];

  if(bWR)
    PDR |= PDR_W;

  if(m == cpu_mode_Invalid) {
    _mmuAbort(PDR, pageNo, s, m, MMU_MMR0_ERR_ABRT_PL | MMU_MMR0_ERR_ABRT_NR);
    return MEM_ERR(MEM_ERR_MMU_ABORTED);
  }

  switch(PDR & PDR_ACF_MASK) {
   case PDR_ACF_NON_RESIDENT:
   case PDR_ACF_UNUSED_3:
   case PDR_ACF_UNUSED_7: {
     _mmuAbort(PDR, pageNo, s, m, MMU_MMR0_ERR_ABRT_PL | MMU_MMR0_ERR_ABRT_NR);
     return MEM_ERR(MEM_ERR_MMU_ABORTED);
   }

   case PDR_ACF_RO_TRAP: {
     if(bWR) {
       _mmuAbort(PDR, pageNo, s, m, MMU_MMR0_ERR_ABRT_RO);
       return MEM_ERR(MEM_ERR_MMU_ABORTED);
     }

     _mmuTrap(PDR);
     break;
   }

   case PDR_ACF_RO: {
     if(bWR) {
       _mmuAbort(PDR, pageNo, s, m, MMU_MMR0_ERR_ABRT_RO);
       return MEM_ERR(MEM_ERR_MMU_ABORTED);
     }

     break;
   }

   case PDR_ACF_RW_TRAP: {
     _mmuTrap(PDR);
     break;
   }

   case PDR_ACF_WR_TRAP: {
     if(bWR)
        _mmuTrap(PDR);

     break;
   }

    case PDR_ACF_RW:
      break;

   default: {
     assert(false);
     break;
   }
  }

  const cpu_word BN = (addr >> 6) & 0x007F;

  if(((PDR & PDR_ED) && (BN < PDR_GET_PLF(PDR))) || (!(PDR & PDR_ED) && (BN > PDR_GET_PLF(PDR)))) {
    _mmuAbort(PDR, pageNo, s, m, MMU_MMR0_ERR_ABRT_PL);
    return MEM_ERR(MEM_ERR_MMU_ABORTED);
  }

  // Page Address Field (PAF) of selected PAR is
  // 12 bits on 11/34A and 11/60 and 16 bits on 11/44 and 11/70.
  // It holds the starting address of the page.
  ph_addr PA = PAR;

  // The Physical Block Number (PBN) is obtained by
  // adding the PAF from PAR to BN from virtual address.
  // PBN will contain our final Physical Address (PA).
  PA += BN;

  // Get final PA by joining 6 bit DIB from VA to PBN.
  PA = (PA << 6) | (addr & 0x003F);

  if(!(_MMR[3] & MMU_MMR3_22BIT_MAP)) {
    // 18-bit mapping
    if(PA >= MEM_18BIT_PERIPH_PAGE_ADDR) {
      PA -= MEM_18BIT_PERIPH_PAGE_ADDR;
      PA += MEM_UNIBUS_PERIPH_PAGE_ADDR + MEM_22BIT_UNIBUS_ADDR;
    }
  }
  else
    assert(false);  // TODO: Implement 22-bit mapping

  return PA;
}

addr_status_t MMU::Map16Bit(cpu_addr addr) {
  // 16-bit mapping

  ph_addr PA = addr;
  if(addr >= MEM_16BIT_PERIPH_PAGE_ADDR) {
    PA -= MEM_16BIT_PERIPH_PAGE_ADDR;
    PA += MEM_UNIBUS_PERIPH_PAGE_ADDR + MEM_22BIT_UNIBUS_ADDR;
  }

  return PA;
}

void MMU::_mmuAbort(cpu_word &PDR, int pageNo, cpu_space s, cpu_mode m, cpu_word errFlags) {
  DEBUG("MMU: ABORT");

  MMU_MMR0_SET_PAGE_NO(_MMR[0], pageNo);
  MMU_MMR0_SET_PAGE_SPACE(_MMR[0], s);
  MMU_MMR0_SET_CPU_MODE_NO(_MMR[0], m);

  _MMR[0] = (_MMR[0] & ~MMU_MMR0_ERR_MASK) | errFlags;

  PDR |= PDR_A;
}

void MMU::_mmuTrap(cpu_word &PDR) {
  DEBUG("MMU: TRAP");

  PDR |= PDR_A;

  _MMR[0] |= MMU_MMR0_TRAP;

  if((_MMR[0] & MMU_MMR0_TRAP_EN) && _cbTrap)
    _cbTrap();
}

std::vector<IoWindow> MMU::_buildIoMap() {
  std::vector<IoWindow> map;

  // Memory management registers
  map.emplace_back(MMU_REG_MMR0, MEM_WORD_SIZE);
  map.emplace_back(MMU_REG_MMR1, MEM_WORD_SIZE);
  map.emplace_back(MMU_REG_MMR2, MEM_WORD_SIZE);
  map.emplace_back(MMU_REG_MMR3, MEM_WORD_SIZE);

  map.emplace_back(MMU_REG_LOWER_SIZE, MEM_WORD_SIZE);
  map.emplace_back(MMU_REG_UPPER_SIZE, MEM_WORD_SIZE);

  for(auto &rg : _mmuGegGroupsTablePDR)
    map.emplace_back(rg.start, rg.size);

  for(auto rg : _mmuGegGroupsTablePAR)
    map.emplace_back(rg.start, rg.size);

  return map;
}
