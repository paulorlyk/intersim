//
// Created by palulukan on 1/10/26.
//

#include "cpu.h"

#include "../log.h"

#include <cassert>

static const char *__arrOpcodeNameLUT[0x10000];
static const char *__arrRegNameLUT[8];
static const char *__arrAddrModePrefixLUT[8];
static const char *__arrAddrModeSuffixLUT[8];

#define BRANCH_IF(cond, offset)           \
  do {                                    \
    if((cond))                            \
    _GPR[7] += (offset) * 2;              \
  } while(0)                              \

#define CALC_V(a, b, r)    ((((a) ^ (b)) & 0x8000) && !(((b) ^ (r)) & 0x8000))

enum _opcode {
  _mov        = 0x1000,   // MOV      Move
  _movb       = 0x9000,   // MOVB     Move
  _cmp        = 0x2000,   // CMP      Compare
  _cmpb       = 0xA000,   // CMPB     Compare
  _bit        = 0x3000,   // BIT      Bit Test
  _bitb       = 0xB000,   // BITB     Bit Test
  _bic        = 0x4000,   // BIC      Bit Clear
  _bicb       = 0xC000,   // BICB     Bit Clear
  _bis        = 0x5000,   // BIS      Bit Set
  _bisb       = 0xD000,   // BISB     Bit Set
  _add        = 0x6000,   // ADD      Add
  _sub        = 0xE000,   // SUB      Subtract
  _mul        = 0x7000,   // MUL      Multiply
  _div        = 0x7200,   // DIV      Divide
  _ash        = 0x7400,   // ASH      Arithmetic Shift
  _ashc       = 0x7600,   // ASHC     Arithmetic Shift Combined
  _xor        = 0x7800,   // XOR      Exclusive OR
  _fp         = 0x7A00,
  _sys        = 0x7C00,
  _sob        = 0x7E00,   // SOB      Subtract one and branch if not equal to 0
  _swab       = 0x00C0,   // SWAB     Swap Bytes
  _clr        = 0x0A00,   // CLR      Clear
  _clrb       = 0x8A00,   // CLRB     Clear
  _com        = 0x0A40,   // COM      Complement
  _comb       = 0x8A40,   // COMB     Complement
  _inc        = 0x0A80,   // INC      Increment
  _incb       = 0x8A80,   // INCB     Increment
  _dec        = 0x0AC0,   // DEC      Decrement
  _decb       = 0x8AC0,   // DECB     Decrement
  _neg        = 0x0B00,   // NEG      Negate
  _negb       = 0x8B00,   // NEGB     Negate
  _adc        = 0x0B40,   // ADC      Add Carry
  _adcb       = 0x8B40,   // ADCB     Add Carry
  _sbc        = 0x0B80,   // SBC      Subtract Carry
  _sbcb       = 0x8B80,   // SBCB     Subtract Carry
  _tst        = 0x0BC0,   // TST      Test
  _tstb       = 0x8BC0,   // TSTB     Test
  _ror        = 0x0C00,   // ROR      Rotate Right
  _rorb       = 0x8C00,   // RORB     Rotate Right
  _rol        = 0x0C40,   // ROL      Rotate Left
  _rolb       = 0x8C40,   // ROLB     Rotate Left
  _asr        = 0x0C80,   // ASR      Arithmetic Shift Right
  _asrb       = 0x8C80,   // ASRB     Arithmetic Shift Right
  _asl        = 0x0CC0,   // ASL      Arithmetic Shift Left
  _aslb       = 0x8CC0,   // ASLB     Arithmetic Shift Left
  _mark       = 0x0D00,   // MARK     Used as part of the standard PDP-11 subroutine return convention.
  _mtps       = 0x8D00,   // MFPS     Move Byte from PSW
  _mfpi       = 0x0D40,   // MFPI     Move from previous instruction space
  _mfpd       = 0x8D40,   // MFPD     Move from previous data space
  _mtpi       = 0x0D80,   // MTPI     Move to previous instruction space
  _mtpd       = 0x8D80,   // MTPD     Move to previous data space
  _sxt        = 0x0DC0,   // SXT      Sign Extend
  _mfps       = 0x8DC0,   // MTPS     Move Byte to PSW
  _br         = 0x0100,   // BR       Branch (Unconditional)
  _bne        = 0x0200,   // BNE      Branch if not equal (to zero)
  _beq        = 0x0300,   // BEQ      Branch if equal (to zero)
  _bge        = 0x0400,   // BGE      Branch if greater than or equal (to zero)
  _blt        = 0x0500,   // BLT      Branch if less than (zero)
  _bgt        = 0x0600,   // BGT      Branch if greater than (zero)
  _ble        = 0x0700,   // BLE      Branch if less than or equal (to zero)
  _bpl        = 0x8000,   // BPL      Branch if plus
  _bmi        = 0x8100,   // BMI      Branch if minus
  _bhi        = 0x8200,   // BHI      Branch if higher
  _blos       = 0x8300,   // BLOS     Branch if lower or same
  _bvc        = 0x8400,   // BVC      Branch if V bit clear
  _bvs        = 0x8500,   // BVS      Branch if V bit set
  _bcc        = 0x8600,   // BCC      Branch if carry clear
  _bcs        = 0x8700,   // BCS      Branch if carry set
  _jmp        = 0x0040,   // JMP      Jump
  _jsr        = 0x0800,   // JSR      Jump to Subroutine
  _rts        = 0x0080,   // RTS      Return from Subroutine
  _reset      = 0x0005,   // RESET    Sends INIT on the UNIBUS for 10ms
  _spl        = 0x0098,   // SPL      Set priority level
  _rti        = 0x0002,   // RTI      Return from Interrupt
  _wait_op    = 0x0001,   // WAIT     Wait for Interrupt
  _emt        = 0x8800,   // EMT      Emulator Trap
  _trap_op    = 0x8900,   // TRAP     Trap
  _c_op       = 0x00A0,   // C        Clear selected condition code bits
  _s_op       = 0x00B0,   // S        Set selected condition codes
  _halt_op    = 0x0000,   // HALT     Halt
  _mfpt_op    = 0x0007,   // MFPT     Move From Processor (PDP-11/44 ONLY)

  // 000003   BPT      Breakpoint Trap
  // 000004   IOT      I/O Trap
  // 000006   RTT      Return from Interrupt

  // 007000   CSM      Call to Supervisor Mode (PDP-11/44 only)

  // 076600   MED      Maintenance, Exam, and Dep

  // 170003   LOUB     Load Microbreak Register
  // 170004   MNS      Maintenance normalization shift
  // 170005   MPP      Maintenance Partial Product
};

enum _instructionType {
  _instt_unknown,
  _instt_single_op,
  _instt_double_op,
  _instt_branch,
  _instt_rts,
  _instt_spl,
  _instt_double_op_reg_src,
  _instt_condition_code,
  _instt_system,
};

struct _instruction{
  _instructionType type;
  _opcode opcode;
  CpuAddressingMode srcMode;
  int src;
  CpuAddressingMode dstMode;
  int dst;
  int reg;
  int offset;
  int mask;
};

static void _initNameLUT() {
  __arrRegNameLUT[0] = "R0";
  __arrRegNameLUT[1] = "R1";
  __arrRegNameLUT[2] = "R2";
  __arrRegNameLUT[3] = "R3";
  __arrRegNameLUT[4] = "R4";
  __arrRegNameLUT[5] = "R5";
  __arrRegNameLUT[6] = "SP";
  __arrRegNameLUT[7] = "PC";

  __arrAddrModePrefixLUT[_reg]                  = "";
  __arrAddrModePrefixLUT[_reg_deferred]         = "(";
  __arrAddrModePrefixLUT[_auto_inc]             = "(";
  __arrAddrModePrefixLUT[_auto_inc_deferred]    = "@(";
  __arrAddrModePrefixLUT[_auto_dec]             = "-(";
  __arrAddrModePrefixLUT[_auto_dec_deferred]    = "@-(";
  __arrAddrModePrefixLUT[_index]                = "X(";
  __arrAddrModePrefixLUT[_index_deferred]       = "@X(";

  __arrAddrModeSuffixLUT[_reg]                  = "";
  __arrAddrModeSuffixLUT[_reg_deferred]         = ")";
  __arrAddrModeSuffixLUT[_auto_inc]             = ")+";
  __arrAddrModeSuffixLUT[_auto_inc_deferred]    = ")+";
  __arrAddrModeSuffixLUT[_auto_dec]             = ")";
  __arrAddrModeSuffixLUT[_auto_dec_deferred]    = ")";
  __arrAddrModeSuffixLUT[_index]                = ")";
  __arrAddrModeSuffixLUT[_index_deferred]       = ")";

  for(size_t i = 0; i < sizeof(__arrOpcodeNameLUT) / sizeof(__arrOpcodeNameLUT[0]); ++i)
    __arrOpcodeNameLUT[i] = "???";

  __arrOpcodeNameLUT[_mov]    = "MOV";
  __arrOpcodeNameLUT[_movb]   = "MOVB";
  __arrOpcodeNameLUT[_cmp]    = "CMP";
  __arrOpcodeNameLUT[_cmpb]   = "CMPB";
  __arrOpcodeNameLUT[_bit]    = "BIT";
  __arrOpcodeNameLUT[_bitb]   = "BITB";
  __arrOpcodeNameLUT[_bic]    = "BIC";
  __arrOpcodeNameLUT[_bicb]   = "BICB";
  __arrOpcodeNameLUT[_bis]    = "BIS";
  __arrOpcodeNameLUT[_bisb]   = "BISB";
  __arrOpcodeNameLUT[_add]    = "ADD";
  __arrOpcodeNameLUT[_sub]    = "SUB";
  __arrOpcodeNameLUT[_mul]    = "MUL";
  __arrOpcodeNameLUT[_div]    = "DIV";
  __arrOpcodeNameLUT[_ash]    = "ASH";
  __arrOpcodeNameLUT[_ashc]   = "ASHC";
  __arrOpcodeNameLUT[_xor]    = "XOR";
  __arrOpcodeNameLUT[_fp]     = "FP";
  __arrOpcodeNameLUT[_sys]    = "SYS";
  __arrOpcodeNameLUT[_sob]    = "SOB";
  __arrOpcodeNameLUT[_swab]   = "SWAB";
  __arrOpcodeNameLUT[_clr]    = "CLR";
  __arrOpcodeNameLUT[_clrb]   = "CLRB";
  __arrOpcodeNameLUT[_com]    = "COM";
  __arrOpcodeNameLUT[_comb]   = "COMB";
  __arrOpcodeNameLUT[_inc]    = "INC";
  __arrOpcodeNameLUT[_incb]   = "INCB";
  __arrOpcodeNameLUT[_dec]    = "DEC";
  __arrOpcodeNameLUT[_decb]   = "DECB";
  __arrOpcodeNameLUT[_neg]    = "NEG";
  __arrOpcodeNameLUT[_negb]   = "NEGB";
  __arrOpcodeNameLUT[_adc]    = "ADC";
  __arrOpcodeNameLUT[_adcb]   = "ADCB";
  __arrOpcodeNameLUT[_sbc]    = "SBC";
  __arrOpcodeNameLUT[_sbcb]   = "SBCB";
  __arrOpcodeNameLUT[_tst]    = "TST";
  __arrOpcodeNameLUT[_tstb]   = "TSTB";
  __arrOpcodeNameLUT[_ror]    = "ROR";
  __arrOpcodeNameLUT[_rorb]   = "RORB";
  __arrOpcodeNameLUT[_rol]    = "ROL";
  __arrOpcodeNameLUT[_rolb]   = "ROLB";
  __arrOpcodeNameLUT[_asr]    = "ASR";
  __arrOpcodeNameLUT[_asrb]   = "ASRB";
  __arrOpcodeNameLUT[_asl]    = "ASL";
  __arrOpcodeNameLUT[_aslb]   = "ASLB";
  __arrOpcodeNameLUT[_mark]   = "MARK";
  __arrOpcodeNameLUT[_mtps]   = "MTPS";
  __arrOpcodeNameLUT[_mfpi]   = "MFPI";
  __arrOpcodeNameLUT[_mfpd]   = "MFPD";
  __arrOpcodeNameLUT[_mtpi]   = "MTPI";
  __arrOpcodeNameLUT[_mtpd]   = "MTPD";
  __arrOpcodeNameLUT[_sxt]    = "SXT";
  __arrOpcodeNameLUT[_mfps]   = "MFPS";
  __arrOpcodeNameLUT[_br]     = "BR";
  __arrOpcodeNameLUT[_bne]    = "BNE";
  __arrOpcodeNameLUT[_beq]    = "BEQ";
  __arrOpcodeNameLUT[_bge]    = "BGE";
  __arrOpcodeNameLUT[_blt]    = "BLT";
  __arrOpcodeNameLUT[_bgt]    = "BGT";
  __arrOpcodeNameLUT[_ble]    = "BLE";
  __arrOpcodeNameLUT[_bpl]    = "BPL";
  __arrOpcodeNameLUT[_bmi]    = "BMI";
  __arrOpcodeNameLUT[_bhi]    = "BHI";
  __arrOpcodeNameLUT[_blos]   = "BLOS";
  __arrOpcodeNameLUT[_bvc]    = "BVC";
  __arrOpcodeNameLUT[_bvs]    = "BVS";
  __arrOpcodeNameLUT[_bcc]    = "BCC/BHIS";
  __arrOpcodeNameLUT[_bcs]    = "BCS/BLO";
  __arrOpcodeNameLUT[_jmp]    = "JMP";
  __arrOpcodeNameLUT[_jsr]    = "JSR";
  __arrOpcodeNameLUT[_rts]    = "RTS";
  __arrOpcodeNameLUT[_reset]  = "RESET";
  __arrOpcodeNameLUT[_spl]    = "SPL";
  __arrOpcodeNameLUT[_rti]    = "RTI";
  __arrOpcodeNameLUT[_wait_op]= "WAIT";
  __arrOpcodeNameLUT[_emt]    = "EMT";
  __arrOpcodeNameLUT[_trap_op]= "TRAP";
  __arrOpcodeNameLUT[_c_op]   = "C";
  __arrOpcodeNameLUT[_s_op]   = "S";
  __arrOpcodeNameLUT[_halt_op]= "HALT";
  __arrOpcodeNameLUT[_mfpt_op]= "MFPT";
}

static std::optional<cpu_word> _attemptRead(cpu_addr addr, cpu_word PSW, Mem* mem, const MMU& mmu) {
  auto pa = mmu.Lookup(addr, cpu_space_I, PSW_GET_CUR_MODE(PSW), false);
  if(!pa)
    return std::nullopt;

  auto w = mem->Read(*pa, false);
  if(w.error)
    return std::nullopt;

  return w.data;
}

static const char* _formatInstructionOperand(CpuAddressingMode mode, int reg, cpu_addr &pc, cpu_word PSW, Mem* mem, const MMU& mmu) {
  static char res[64] = "";

  int pos = 0;

  if(reg == 7) {
    switch(mode) {
      default:
        break;

      case _auto_inc: {   // PC Immediate
        auto data = _attemptRead(pc += 2, PSW, mem, mmu);
        if(!data)
          pos += sprintf(res + pos, "#?[MEM_FAULT]");
        else
          pos += sprintf(res + pos, "#%06o", *data);

        return res;
      }

      case _auto_inc_deferred: {  // PC Absolute
        auto data = _attemptRead(pc += 2, PSW, mem, mmu);
        if(!data)
          pos += sprintf(res + pos, "@#?[MEM_FAULT]");
        else
          pos += sprintf(res + pos, "@#%06o", *data);

        return res;
      }
    }
  }

  switch(mode) {
    default:
      break;

    case _index: {
      auto data = _attemptRead(pc += 2, PSW, mem, mmu);
      if(!data)
        pos += sprintf(res + pos, "X[MEM_FAULT](%s)", __arrRegNameLUT[reg]);
      else
        pos += sprintf(res + pos, "#%06o(%s)", *data, __arrRegNameLUT[reg]);

      return res;
    }

    case _index_deferred: {
      auto data = _attemptRead(pc += 2, PSW, mem, mmu);
      if(!data)
        pos += sprintf(res + pos, "@X[MEM_FAULT](%s)", __arrRegNameLUT[reg]);
      else
        pos += sprintf(res + pos, "@#%06o(%s)", *data, __arrRegNameLUT[reg]);

      return res;
    }
  }

  sprintf(res, "%s%s%s", __arrAddrModePrefixLUT[mode], __arrRegNameLUT[reg], __arrAddrModeSuffixLUT[mode]);

  return res;
}

static const char* _formatInstruction(cpu_addr pc, cpu_word instWord, const struct _instruction* pInst, cpu_word PSW, Mem* mem, const MMU& mmu) {
  static char res[256] = "";

  pc -= 2;

  int pos = sprintf(res, "%06o: %06o\t%s", pc, instWord, __arrOpcodeNameLUT[pInst->opcode]);

  switch(pInst->type) {
    default:
    case _instt_unknown:
      pos += sprintf(res + pos, " ?");
      break;

    case _instt_single_op:
      pos += sprintf(res + pos, " %s", _formatInstructionOperand(pInst->dstMode, pInst->dst, pc, PSW, mem, mmu));
      break;

    case _instt_double_op:
      pos += sprintf(res + pos, " %s", _formatInstructionOperand(pInst->srcMode, pInst->src, pc, PSW, mem, mmu));
      pos += sprintf(res + pos, ", %s", _formatInstructionOperand(pInst->dstMode, pInst->dst, pc, PSW, mem, mmu));
      break;

    case _instt_branch:
      pos += sprintf(res + pos, " .%+d", pInst->offset);
      break;

    case _instt_rts:
      pos += sprintf(res + pos, " %s", __arrRegNameLUT[pInst->dst]);
      break;

    case _instt_spl:
      pos += sprintf(res + pos, " %o", pInst->dst);
      break;

    case _instt_double_op_reg_src:
      pos += sprintf(res + pos, " %s", _formatInstructionOperand(pInst->srcMode, pInst->src, pc, PSW, mem, mmu));
      pos += sprintf(res + pos, ", %s", __arrRegNameLUT[pInst->reg]);
      break;

    case _instt_condition_code:
      pos += sprintf(res + pos, " 0%03o", pInst->mask);
      break;

    case _instt_system:
      break;
  }

  return res;
}

CPU::CPU(Mem *mem, cpu_word R7):
  UnibusDevice({
    { .start = 0777776, .size = MEM_WORD_SIZE },  // PS
    { .start = 0777570, .size = MEM_WORD_SIZE },  // Console Switch & Display Register
    // { .start = 0777764, .size = MEM_WORD_SIZE },  // System I/D Register
  }, 0, "PDP-11/70 CPU"),
  _mem(mem)
{
  _setPSW(0);

  _initNameLUT();

  _GPR[7] = R7;

  _mem->GetUnibus()->RegisterDevice(this);
  _mem->GetUnibus()->RegisterDevice(&_mmu);

  _mmu.OnTrap([this]() {
    _mmuTrap = true;
  });

  // Indicates that the current instruction has been completed.
  // It will be set to 0 during T bit, Parity, Odd Address, and Time Out traps and interrupts.
  // This provides error handling routines with a way of determining whether
  // the last instruction will have to be repeated in the course of an error
  // recovery attempt. Bit 7 is read-only (it cannot be written). It is initialized to a 1.
  // Note that EMT, TRAP, BPT, and lOT do not set bit 7.
  _mmu.UpdateMMR0(true);
}

CPU::~CPU() {
  _mem->GetUnibus()->UnregisterDevice(&_mmu);
  _mem->GetUnibus()->UnregisterDevice(this);
}

cpu_word CPU::IrqAck() {
  return 0;
}

bool CPU::Run() {
  // TODO: Debug
  // if(_GPR[7] == 0001000) {
  //   _disassemblyOutput = true;
  //   DEBUG("debug");
  // }
  // _disassemblyOutput = true;

  if(_halt)
    return false;

  if(_mmuTrap) {
    DEBUG("Processing MMU TRAP");
    _trap(TRAP_MMU);
  } else {
    auto irqDev = _mem->GetUnibus()->GetIRQ(PSW_GET_PRIORITY(_PSW));
    if(irqDev) {
      cpu_word vec = _mem->GetUnibus()->AckIRQ();

      // DEBUG("Processing IRQ from device: %s, vector: 0%03o", dev_getName(hIRQDev), vec);
      _trap(vec);
    }
  }

  if(_wait)
    return false;

  // MMR1 records any auto increment/decrement of the general purpose
  // registers, including explicit references through the PC. MMR1 is
  // cleared at the beginning of each instruction fetch. Whenever a general
  // purpose register is either autoincremented or autodecremented, the
  // register number and the amount by which the register was modified
  // (in 2's complement notation) is written into MMR1.
  _mmu.ResetMMR1();

  // MMR2 is loaded with the 16-bit Virtual Address (VA) at the beginning
  // of each instruction fetch, or with the address Trap Vector at the
  // beginning of an interrupt, T Bit trap, Parity, Odd Address, and Timeout
  // aborts and parity traps. Note that MMR2 does not get the Trap Vector
  // on EMT, TRAP, BPT and lOT instructions.
  _mmu.UpdateMMR2(_GPR[7]);

  cpu_word instWord;
  if(!_fetchPC(&instWord))
    return !_wait;

  // if(instWord == 005067) {
  //   _disassemblyOutput = true;
  //   DEBUG("debug");
  // }

  // DEBUG("Instruction fetch: 0%06o: 0%06o", _GPR[7] - 2, instWord);

  _instruction inst = {};
  if(!_decode(instWord, &inst)) {
    DEBUG("UNKNOWN INSTRUCTION: 0%06o: 0%06o", _GPR[7] - 2, instWord);

    _trap(TRAP_RESERVED_INSTRUCTION);
    return !_wait;
  }

  // TODO: Debug
  if(_disassemblyOutput) {
    static int ctr = 0;
    DEBUG("%d %s", ctr++, _formatInstruction(_GPR[7], instWord, &inst, _PSW, _mem, _mmu));

    // if(ctr >= 2000)
    //   DEBUG("debug");
  }

  cpu_word srcVal = 0;
  operand_addr srcAddr = 0;

  cpu_word dstVal = 0;
  operand_addr dstAddr = 0;

  bool byteFlag = inst.opcode & 0x8000;

  switch(inst.opcode) {
    case _mov:
    case _movb: {
      if(   _load(inst.srcMode, inst.src, byteFlag, &srcAddr, &srcVal)
         && _makeOperandAddress(inst.dstMode, inst.dst, byteFlag, &dstAddr)
         && _store(dstAddr, srcVal)) {
        _setFlags(srcVal & 0x8000, !srcVal, 0, PSW_GET_C(_PSW));
      }
      break;
    }

    case _cmp:
    case _cmpb: {
      if(   _load(inst.srcMode, inst.src, byteFlag, &srcAddr, &srcVal)
         && _load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        uint32_t res = (uint32_t)srcVal - dstVal;
        _setFlags(res & 0x8000, !res, CALC_V(srcVal, dstVal, res), res & 0x10000);
      }
      break;
    }

    case _bit:
    case _bitb: {
      if(   _load(inst.srcMode, inst.src, byteFlag, &srcAddr, &srcVal)
         && _load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        dstVal &= srcVal;
        _setFlags(dstVal & 0x8000, !dstVal, 0, PSW_GET_C(_PSW));
      }
      break;
    }

    case _bic:
    case _bicb: {
      if(   _load(inst.srcMode, inst.src, byteFlag, &srcAddr, &srcVal)
         && _load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        dstVal &= ~srcVal;
        if(_store(dstAddr, dstVal))
          _setFlags(dstVal & 0x8000, !dstVal, 0, PSW_GET_C(_PSW));
      }
      break;
    }

    case _bis:
    case _bisb: {
      if(   _load(inst.srcMode, inst.src, byteFlag, &srcAddr, &srcVal)
         && _load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        dstVal |= srcVal;
        if(_store(dstAddr, dstVal))
          _setFlags(dstVal & 0x8000, !dstVal, 0, PSW_GET_C(_PSW));
      }
      break;
    }

    case _add: {
      if(   _load(inst.srcMode, inst.src, false, &srcAddr, &srcVal)
         && _load(inst.dstMode, inst.dst, false, &dstAddr, &dstVal)) {
        uint32_t res = (uint32_t)srcVal + dstVal;
        if(_store(dstAddr, res))
          _setFlags(res & 0x8000, !res, CALC_V(srcVal, dstVal, res), res & 0x10000);
      }
      break;
    }

    case _sub: {
      if(   _load(inst.srcMode, inst.src, false, &srcAddr, &srcVal)
         && _load(inst.dstMode, inst.dst, false, &dstAddr, &dstVal)) {
        uint32_t res = (uint32_t)dstVal - srcVal;
        if(_store(dstAddr, res))
          _setFlags(res & 0x8000, !res, CALC_V(dstVal, srcVal, res), res & 0x10000);
      }
      break;
    }

    case _mul: {
      if(!_load(inst.srcMode, inst.src, false, &srcAddr, &srcVal))
        break;
      dstVal = _GPR[inst.reg];

      // Convert to signed integer
      int32_t src = ((int32_t)(srcVal & 0x7FFF)) - ((int32_t)(srcVal & 0x8000));
      int32_t dst = ((int32_t)(dstVal & 0x7FFF)) - ((int32_t)(dstVal & 0x8000));

      int32_t mul = src * dst;
      _GPR[inst.reg] = mul >> 16;
      _GPR[inst.reg | 1] = mul;

      _setFlags(mul & 0x80000000, !mul, 0, mul < -32768 || mul >= 32768);
      break;
    }

    case _div: {
      if(!_load(inst.srcMode, inst.src, false, &srcAddr, &srcVal))
        break;

      if(srcVal != 0) {
        // Convert to signed integer
        int32_t den = ((int32_t)(srcVal & 0x7FFF)) - ((int32_t)(srcVal & 0x8000));

        // Assemble 32 bit value
        uint32_t arg = _GPR[inst.reg | 1] | (((uint32_t)_GPR[inst.reg]) << 16);
        // Convert to signed integer
        int32_t num = ((int32_t)(arg & 0x7FFFFFFF)) - ((int32_t)(arg & 0x80000000ULL));

        int32_t quot = num / den;
        int32_t rem = num % den;
        if(quot < 0x8000 && quot > -0x10000) {
          _GPR[inst.reg] = quot & 0xFFFF;
          _GPR[inst.reg | 1] = rem & 0xFFFF;

          _setFlags(quot < 0, !quot, 0, 0);
        } else {
          _setFlags(0, 0, 1, 0);  // Result is unrepresentable in 16 bit register
        }
      } else {
        _setFlags(0, 0, 1, 1);  // Division by 0
      }
      break;
    }

    case _ash: {
      if(!_load(inst.srcMode, inst.src, false, &srcAddr, &srcVal))
        break;

      dstVal = _GPR[inst.reg];

      // The shift bits count ranges from -32 (right shift)
      // to +31 (left shift). Choose 64bit uints here to
      // avoid undefined behavior while shifting.
      uint64_t res = dstVal;
      bool c = PSW_GET_C(_PSW);

      if(srcVal & 0x0020) {
        // Negative - right shift
        int n = 0x0040 - (srcVal & 0x003F);
        res >>= n;
        if(dstVal & 0x8000)
          res |= ~(0x7FFFULL >> n);
        c = dstVal & (1ULL << (n - 1));
      } else if(srcVal & 0x003F) {
        // Positive - right shift
        int n = srcVal & 0x003F;
        res <<= n;
        c = dstVal & (0x8000ULL >> (n - 1));
      }

      _GPR[inst.reg] = res;
      _setFlags(res & 0x8000, !(res & 0xFFFF), (res ^ dstVal) & 0x8000, c);
      break;
    }

    case _ashc: {
      if(!_load(inst.srcMode, inst.src, false, &srcAddr, &srcVal))
        break;

      uint64_t dest = (((uint64_t)_GPR[inst.reg]) << 16) | _GPR[inst.reg | 1];

      // The shift bits count ranges from -32 (right shift)
      // to +31 (left shift). Choose 64bit uints here to
      // avoid undefined behavior while shifting.
      uint64_t res = dest;
      bool c = PSW_GET_C(_PSW);

      if(srcVal & 0x0020) {
        // Negative - right shift
        int n = 0x0040 - (srcVal & 0x003F);
        res >>= n;
        if(dest & 0x80000000)
          res |= ~(0x7FFFFFFFULL >> n);
        c = dest & (1ULL << (n - 1));
      } else if(srcVal & 0x003F) {
        // Positive - right shift
        int n = srcVal & 0x003F;
        res <<= n;
        c = dest & (0x80000000ULL >> (n - 1));
      }

      _GPR[inst.reg] = res >> 16;
      _GPR[inst.reg | 1] = res;

      _setFlags(res & 0x80000000, !(res & 0xFFFFFFFF), (res ^ dest) & 0x80000000, c);
      break;
    }

    case _xor: assert(false);

    case _fp: assert(false);

    case _sys: assert(false);

    case _sob: {
      --_GPR[inst.reg];
      BRANCH_IF(_GPR[inst.reg], inst.offset);
      break;
    }

    case _swab: {
      if(_load(inst.dstMode, inst.dst, false, &dstAddr, &dstVal)) {
        cpu_word res = (dstVal << 8) | (dstVal >> 8);
        if(_store(dstAddr, res))
          _setFlags(res & 0x0080, !(res & 0xFF), 0, 0);
      }
      break;
    }

    case _clr:
    case _clrb: {
      if(_makeOperandAddress(inst.dstMode, inst.dst, byteFlag, &dstAddr) && _store(dstAddr, 0))
        _setFlags(0, 1, 0, 0);
      break;
    }

    case _com: assert(false);
    case _comb: assert(false);

    case _inc:
    case _incb: {
      if(_load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        ++dstVal;
        if(_store(dstAddr, dstVal))
          _setFlags(dstVal & 0x8000, !dstVal, dstVal == 0x8000, PSW_GET_C(_PSW));
      }
      break;
    }

    case _dec:
    case _decb: {
      if(_load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        --dstVal;
        if(_store(dstAddr, dstVal))
          _setFlags(dstVal & 0x8000, !dstVal, dstVal == 0x7FFF, PSW_GET_C(_PSW));
      }
      break;
    }

    case _neg:
    case _negb: {
      if(_load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        cpu_word res = ~dstVal + 1;
        if(_store(dstAddr, res))
          _setFlags(res & 0x8000, !res, res == 0x8000, res);
      }
      break;
    }

    case _adc:
    case _adcb: {
      if(_load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        uint32_t res = (uint32_t)dstVal + PSW_GET_C(_PSW);
        if(_store(dstAddr, res))
          _setFlags(res & 0x8000, !res, dstVal == 0x7FFF && PSW_GET_C(_PSW), dstVal == 0xFFFF && PSW_GET_C(_PSW));
      }
      break;
    }

    case _sbc:
    case _sbcb: {
      if(_load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        uint32_t res = (uint32_t)dstVal - PSW_GET_C(_PSW);
        if(_store(dstAddr, res))
          _setFlags(res & 0x8000, !res, res == 0x8000, res || !PSW_GET_C(_PSW));
      }
      break;
    }

    case _tst:
    case _tstb: {
        if(_load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal))
          _setFlags(dstVal & 0x8000, !dstVal, 0, 0);
        break;
    }

    case _ror:
    case _rorb: {
      if(_load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        cpu_word res = (dstVal >> 1) | (PSW_GET_C(_PSW) << 15);
        if(_store(dstAddr, res)) {
          bool n = res & 0x8000;
          bool c = dstVal & 1;
          _setFlags(n, !res, n != c, c);
        }
      }
      break;
    }

    case _rol:
    case _rolb: {
      if(_load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        uint32_t res = (dstVal << 1) | PSW_GET_C(_PSW);
        if(_store(dstAddr, res)) {
          bool n = res & 0x8000;
          bool c = res & 0x10000;
          _setFlags(n, !res, n != c, c);
        }
      }
      break;
    }

    case _asr:
    case _asrb: {
      if(_load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        cpu_word res = (dstVal >> 1) | (dstVal & 0x8000);
        if(_store(dstAddr, res)) {
          bool n = res & 0x8000;
          bool c = dstVal & 1;
          _setFlags(n, !res, n != c, c);
        }
      }
      break;
    }

    case _asl:
    case _aslb: {
      if(_load(inst.dstMode, inst.dst, byteFlag, &dstAddr, &dstVal)) {
        uint32_t res = (uint32_t)dstVal << 1;
        if(_store(dstAddr, res)) {
          bool n = res & 0x8000;
          bool c = res & 0x10000;
          _setFlags(n, !res, n != c, c);
        }
      }
      break;
    }

    case _mark: assert(false);

    case _mtps: assert(false);

    case _mfpi: {
      if(!_makeOperandAddress(inst.dstMode, inst.dst, false, &dstAddr))
        break;

      if(dstAddr & OPERAND_TYPE_REG) {
        cpu_word psw = _PSW;
        _setPSW(PSW_SET_CUR_MODE(psw, PSW_GET_PREV_MODE(psw)));
        dstVal = _GPR[inst.dst];
        _setPSW(psw);
      } else if(!_read(dstAddr, false, cpu_space_I, PSW_GET_PREV_MODE(_PSW), &dstVal)) {
        break;
      }

      if(_push(dstVal))
        _setFlags(dstVal & 0x8000, !dstVal, 0, PSW_GET_C(_PSW));
      break;
    }

    case _mfpd: assert(false);

    case _mtpi: {
      if(!_makeOperandAddress(inst.dstMode, inst.dst, false, &dstAddr) || !_pop(&dstVal))
        break;

      if(dstAddr & OPERAND_TYPE_REG) {
        cpu_word psw = _PSW;
        _setPSW(PSW_SET_CUR_MODE(psw, PSW_GET_PREV_MODE(psw)));
        _GPR[inst.dst] = dstVal;
        _setPSW(psw);
      } else if(!_write(dstAddr, false, dstVal, cpu_space_I, PSW_GET_PREV_MODE(_PSW))) {
        break;
      }

      _setFlags(dstVal & 0x8000, !dstVal, 0, PSW_GET_C(_PSW));
      break;
    }

    case _mtpd: assert(false);

    case _sxt: {
      _GPR[inst.dst] = PSW_GET_N(_PSW) ? 0xFFFF : 0;
      _setFlags(PSW_GET_N(_PSW), !PSW_GET_N(_PSW), 0, PSW_GET_C(_PSW));
      break;
    }

    case _mfps: assert(false);

    case _br:   BRANCH_IF(true,                                                       inst.offset); break;
    case _bne:  BRANCH_IF(!PSW_GET_Z(_PSW),                                           inst.offset); break;
    case _beq:  BRANCH_IF(PSW_GET_Z(_PSW),                                            inst.offset); break;
    case _bge:  BRANCH_IF(PSW_GET_N(_PSW) == PSW_GET_V(_PSW),                         inst.offset); break;
    case _blt:  BRANCH_IF(PSW_GET_N(_PSW) != PSW_GET_V(_PSW),                         inst.offset); break;
    case _bgt:  BRANCH_IF(!PSW_GET_Z(_PSW) && ((PSW_GET_N(_PSW) == PSW_GET_V(_PSW))), inst.offset); break;
    case _ble:  BRANCH_IF(PSW_GET_Z(_PSW) || ((PSW_GET_N(_PSW) != PSW_GET_V(_PSW))),  inst.offset); break;
    case _bpl:  BRANCH_IF(!PSW_GET_N(_PSW),                                           inst.offset); break;
    case _bmi:  BRANCH_IF(PSW_GET_N(_PSW),                                            inst.offset); break;
    case _bhi:  BRANCH_IF(!PSW_GET_C(_PSW) && !PSW_GET_Z(_PSW),                       inst.offset); break;
    case _blos: BRANCH_IF(PSW_GET_C(_PSW) || PSW_GET_Z(_PSW),                         inst.offset); break;
    case _bvc:  BRANCH_IF(!PSW_GET_V(_PSW),                                           inst.offset); break;
    case _bvs:  BRANCH_IF(PSW_GET_V(_PSW),                                            inst.offset); break;
    case _bcc:  BRANCH_IF(!PSW_GET_C(_PSW),                                           inst.offset); break;
    case _bcs:  BRANCH_IF(PSW_GET_C(_PSW),                                            inst.offset); break;

    case _jmp: {
      if(_makeOperandAddress(inst.dstMode, inst.dst, byteFlag, &dstAddr)) {
        if(dstAddr & OPERAND_TYPE_REG) {
          // TODO: Illegal instruction trap
          assert(false);
        } else {
          _GPR[7] = dstAddr;
        }
      }
      break;
    }

    case _jsr: {
      if(_makeOperandAddress(inst.dstMode, inst.dst, byteFlag, &dstAddr)) {
        if(dstAddr & OPERAND_TYPE_REG) {
          // TODO: Illegal instruction trap
          assert(false);
        }
        else if(_push(_GPR[inst.src])){
          _GPR[inst.src] = _GPR[7];
          _GPR[7] = dstAddr;
        }
      }
      break;
    }

    case _rts: {
      _GPR[7] = _GPR[inst.dst];
      _pop(_GPR + inst.dst);
      break;
    }

    case _reset: {
      if(PSW_GET_CUR_MODE(_PSW) == CPU_MODE_KERNEL) {
        // Also resets MMU trough Unibus
        _mem->Reset();
      }
      break;
    }

    case _spl: {
      if(PSW_GET_CUR_MODE(_PSW) == CPU_MODE_KERNEL)
        _setPSW(PSW_SET_PRIORITY(_PSW, inst.dst));
      break;
    }

    case _rti: {
      cpu_word PC;
      cpu_word psw;
      if(_pop(&PC) && _pop(&psw)) {
        psw &= PSW_MASK;
        if(PSW_GET_CUR_MODE(_PSW) != CPU_MODE_KERNEL) {
          // When executed in Supervisor mode, the current and previous
          // mode bits in the restored PS cannot be Kernel. When executed in
          // User mode, the current and previous mode bits in the restored PS
          // can only be User. RTI cannot clear PS<11> if it was already set.
          // Apparently priority level is also not allowed to be lowered...
          psw = (psw & 0xF81F) | (_PSW & 0xF8E0);
        }

        _GPR[7] = PC;
        _setPSW(psw);

        // TODO: T bit
        assert(PSW_GET_TRAP(_PSW) == 0);
      }

      // DEBUG("RTI to 0%06o", cpu.GPR[7]);
      break;
    }

    case _wait_op: {
      _wait = true;
      break;
    }

    case _halt_op: {
      _halt = true;
      break;
    }

    case _emt: {
      _trap(TRAP_EMT);
      break;
    }

    case _trap_op: {
      _trap(TRAP_TRAP);
      break;
    }

    case _c_op: {
      _setFlags(
        PSW_GET_N(_PSW) && !(inst.mask & (1 << 3)),
        PSW_GET_Z(_PSW) && !(inst.mask & (1 << 2)),
        PSW_GET_V(_PSW) && !(inst.mask & (1 << 1)),
        PSW_GET_C(_PSW) && !(inst.mask & (1 << 0))
      );
      break;
    }

    case _s_op: {
      _setFlags(
        PSW_GET_N(_PSW) || (inst.mask & (1 << 3)),
        PSW_GET_Z(_PSW) || (inst.mask & (1 << 2)),
        PSW_GET_V(_PSW) || (inst.mask & (1 << 1)),
        PSW_GET_C(_PSW) || (inst.mask & (1 << 0))
      );
      break;
    }

    case _mfpt_op: {
      // PDP-11/44 only
      DEBUG("MFPT INSTRUCTION: %s", _formatInstruction(_GPR[7], instWord, &inst, _PSW, _mem, _mmu));
      _trap(TRAP_RESERVED_INSTRUCTION);
      break;
    }

    default: {
      DEBUG("UNIMPLEMENTED INSTRUCTION: %s", _formatInstruction(_GPR[7], instWord, &inst, _PSW, _mem, _mmu));
      assert(false);
      break;
    }
  }

  return !_wait;
}

void CPU::_trap(cpu_word vec) {
  if(_inTrap) {
    DEBUG("TRAP IN TRAP %u @ 0%06o", vec, _GPR[7]);

    // TODO: Trap from trap
    assert(false);
  }

  // TODO: Debug
  // if(vec != 28 && vec != 144 && vec != 64 && vec != 48 && vec != 52)
  //   DEBUG("TRAP 0%o @ 0%06o", vec, _GPR[7]);

  // TODO: Debug
  // _disassemblyOutput = true;

  _inTrap = true;
  _wait = false;

  _mmu.UpdateMMR0(false);
  _mmu.UpdateMMR2(vec);

  cpu_word newPC;
  cpu_word newPSW;
  if(   _read(vec, false, cpu_space_D, cpu_mode_Kernel, &newPC)
     && _read(vec + 2, false, cpu_space_D, cpu_mode_Kernel, &newPSW)) {
    cpu_word oldPSW = _PSW;
    cpu_word oldPC = _GPR[7];

    _setPSW(PSW_SET_PREV_MODE(newPSW, PSW_GET_CUR_MODE(oldPSW)));
    _GPR[7] = newPC;

    if(_push(oldPSW))
      _push(oldPC);
  }

  _mmu.UpdateMMR0(true);
  _inTrap = false;
}

void CPU::_setPSW(cpu_word psw) {
  _lastSP[PSW_GET_CUR_MODE(_PSW)] = _GPR[6];
  auto lastPC = _GPR[7];

  _GPR = _regSet[PSW_GET_REG_SET(psw)];
  _GPR[6] = _lastSP[PSW_GET_CUR_MODE(psw)];
  _GPR[7] = lastPC;

  _PSW = psw;
}

bool CPU::_read(cpu_addr addr, bool bByte, cpu_space space, cpu_mode mode, cpu_word *data) {
  auto pa = _mmu.Map(addr, space, mode, false);
  if(!pa) {
    DEBUG("CPU MMU READ ERROR");

    // TODO: Update CPU Error Register

    _trap(TRAP_MMU);
    return false;
  }

  auto w = _mem->Read(*pa, bByte);
  if(w.error) {
    DEBUG("CPU MEM READ ERROR: 0x%08x", w.data);

    // TODO: Update CPU Error Register

    _trap(TRAP_ERR);
    return false;
  }

  *data = bByte ? SIGN_EXTEND_BYTE(w.data) : w.data;

  return true;
}

bool CPU::_write(cpu_addr addr, bool bByte, cpu_word data, cpu_space space, cpu_mode mode) {
  auto pa = _mmu.Map(addr, space, mode, true);
  if(!pa) {
    DEBUG("CPU MMU WRITE ERROR");

    // TODO: Update CPU Error Register

    _trap(TRAP_MMU);
    return false;
  }

  auto res = _mem->Write(*pa, bByte, data);
  if(res.error) {
    DEBUG("CPU MEM WRITE ERROR: 0x%08x", res.data);

    // TODO: Update CPU Error Register

    _trap(TRAP_ERR);
    return false;
  }

  return true;
}

bool CPU::_makeOperandAddress(CpuAddressingMode mode, int reg, bool bByte, operand_addr *pAddr) {
  assert(mode >= _reg && mode <= _index_deferred);
  assert(reg >= 0 && reg < 8);

  cpu_word *pRn = _GPR + reg;
  int nRnDiff = 0;
  cpu_word addr;

  *pAddr = 0;

  switch(mode) {
    default:
    case _reg: {
      *pAddr |= OPERAND_TYPE_REG;
      addr = reg;
      break;
    }

    case _reg_deferred: {
      addr = *pRn;
      break;
    }

    case _auto_inc: {
      addr = *pRn;
      *pRn += nRnDiff = 1 + (!bByte || reg == 6 || reg == 7);
      break;
    }

    case _auto_inc_deferred: {
      if(!_read(*pRn, false, cpu_space_D, PSW_GET_CUR_MODE(_PSW), &addr))
        return false;

      *pRn += nRnDiff = 2;
      break;
    }

    case _auto_dec: {
      *pRn += nRnDiff = -(1 + (!bByte || reg == 6 || reg == 7));
      addr = *pRn;
      break;
    }

    case _auto_dec_deferred: {
      nRnDiff = -2;

      if(!_read(*pRn, false, cpu_space_D, PSW_GET_CUR_MODE(_PSW), &addr))
        return false;

      *pRn += nRnDiff;
      break;
    }

    case _index: {
      // Make sure that index was fetched before reading register value
      // This is important in case if register itself is PC
      cpu_word X;
      nRnDiff = _fetchPC(&X);
      if(!nRnDiff)
        return false;

      addr = *pRn + X;
      break;
    }

    case _index_deferred: {
      // Make sure that index was fetched before reading register value
      // This is important in case if register itself is PC
      cpu_word X;
      nRnDiff = _fetchPC(&X);
      if(!nRnDiff)
        return false;

      if(!_read(*pRn + X, false, cpu_space_D, PSW_GET_CUR_MODE(_PSW), &addr))
        return false;
      break;
    }
  }

  *pAddr |= addr;
  if(bByte)
    *pAddr |= OPERAND_TYPE_BYTE;

  _mmu.UpdateMMR1(reg, nRnDiff);

  return true;
}

bool CPU::_load(CpuAddressingMode mode, int reg, bool bByte, operand_addr *pAddr, cpu_word *data) {
  if(!_makeOperandAddress(mode, reg, bByte, pAddr))
    return false;

  if(*pAddr & OPERAND_TYPE_REG) {
    *data = bByte ? SIGN_EXTEND_BYTE(_GPR[reg]) : _GPR[reg];
    return true;
  }

  return _read(*pAddr, bByte, cpu_space_D, PSW_GET_CUR_MODE(_PSW), data);
}

bool CPU::_store(operand_addr addr, cpu_word data) {
  if(addr & OPERAND_TYPE_REG) {
    _GPR[addr & 7] = data;
    return true;
  }

  return _write(addr, addr & OPERAND_TYPE_BYTE, data, cpu_space_D, PSW_GET_CUR_MODE(_PSW));
}

bool CPU::_push(cpu_word w) {
  operand_addr addr;
  return _makeOperandAddress(_auto_dec, 6, false, &addr) && _store(addr, w);
}

bool CPU::_pop(cpu_word *w) {
  operand_addr addr;
  return _load(_auto_inc, 6, false, &addr, w);
}

void CPU::_setFlags(bool n, bool z, bool v, bool c) {
  _PSW = (_PSW & ~0x000F) | (n << 3) | (z << 2) | (v << 1) | (c << 0);
}

int CPU::_fetchPC(cpu_word *data) {
  if(_GPR[7] & 1) {
    // TODO: Boundary error trap
    assert(false);

    return 0;
  }

  if(!_read(_GPR[7], false, cpu_space_I, PSW_GET_CUR_MODE(_PSW), data))
    return false;

  _GPR[7] += 2;

  return 2;
}

bool CPU::_decode(cpu_word inst, struct _instruction *pInst) {
  // TODO: Debug
  pInst->opcode = (_opcode)0xFFFFFF;

  pInst->type = _instt_unknown;

  if((inst & 0x7000) == 0x0000) {         // x000 xxxx xxxx xxxx
    if((inst & 0x7800) == 0x0800) {       // x000 1xxx xxxx xxxx
      if((inst & 0x7E00) == 0x0800) {     // x000 100x xxxx xxxx
        if((inst & 0xFE00) == 0x0800) {   // 0000 100x xxxx xxxx
          // JSR
          //  15    9   8     6   5  3   2      0
          // [0000100] [LinkReg] [Mode] [Register]

          //DEBUG("Decoder: Jsr instruction");

          pInst->type = _instt_double_op;
          pInst->opcode = (_opcode)(inst & 0xFE00);
          pInst->srcMode = _reg;
          pInst->src = (inst & 0x01C0) >> 6;
          pInst->dstMode = (CpuAddressingMode)((inst & 0x0038) >> 3);
          pInst->dst = (inst & 0x0007) >> 0;
        } else {   // 1000 100x xxxx xxxx
          // EMT / TRAP
          //  15    9   8        7  0
          // [1000100] [Opcode] [NNNN]
          //
          // Opcode   Mnemonic
          // 1040     EMT
          // 1044     TRAP

          //DEBUG("Decoder: EMT/TRAP instruction");

          pInst->type = _instt_system;
          pInst->opcode = (_opcode)(inst & 0x8900);
        }
      } else if((inst & 0x7E00) == 0x0E00) {  // x000 111x xxxx xxxx
        DEBUG("UNKNOWN INSTRUCTION: 0%06o", inst);

        assert(false);
      } else {  // x000 1IIx xxxx xxxx; II != 0 && II != 3
        // Single-operand instructions
        //  15  14 11  10   6   5  3   2      0
        // [B] [0001] [Opcode] [Mode] [Register]
        //
        // Opcode   Mnemonic
        // 010      CLR / CLRB
        // 011      COM / COMB
        // 012      INC / INCB
        // 013      DEC / DECB
        // 014      NEG / NEGB
        // 015      ADC / ADCB
        // 016      SBC / SBCB
        // 017      TST / TSTB
        // 020      ROR / RORB
        // 021      ROL / ROLB
        // 022      ASR / ASRB
        // 023      ASL / ASLB
        // 024      MARK / MTPS
        // 025      MFPI / MFPD
        // 026      MTPI / MTPD
        // 027      SXT / MFPS

        //DEBUG("Decoder: Single-operand instruction");

        pInst->type = _instt_single_op;
        pInst->opcode = (_opcode)(inst & 0xFFC0);
        pInst->dstMode = (CpuAddressingMode)((inst & 0x0038) >> 3);
        pInst->dst = (inst & 0x0007) >> 0;
      }
    } else if((inst & 0x7800) == 0x0000) {  // x000 0xxx xxxx xxxx
      if((inst & 0xFF00) == 0x0000) { // 0000 0000 xxxx xxxx
        if((inst & 0xFFC0) == 0x0040 || (inst & 0xFFC0) == 0x00C0) {  // 0000 0000 IIxx xxxx; II == 1 || II == 3
          // Single-operand instructions
          //  15            8   7    6   5  3   2      0
          // [0 0 0 0 0 0 0 0] [Opcode] [Mode] [Register]
          //
          // Opcode   Mnemonic
          // 01       JMP
          // 03       SWAB

          //DEBUG("Decoder: JMP/SWAB instruction");

          pInst->type = _instt_single_op;
          pInst->opcode = (_opcode)(inst & 0xFFC0);
          pInst->dstMode = (CpuAddressingMode)((inst & 0x0038) >> 3);
          pInst->dst = (inst & 0x0007) >> 0;
        } else if((inst & 0xFFF8) == 0x0080 || (inst & 0xFFF8) == 0x0098) { // 0000 0000 100I Ixxx; II == 0 || II == 3
          // RTS
          // SPL
          //  15                      3   2      0
          // [0 0 0 0 0 0 0 0 1 0 0 0 0] [Register]

          //DEBUG("Decoder: RTS/SPL instruction");

          pInst->type = (inst & 0xFFF8) == 0x0080 ? _instt_rts : _instt_spl;
          pInst->opcode = (_opcode)(inst & 0xFFF8);
          pInst->dst = (inst & 0x0007) >> 0;
        } else if((inst & 0xFFF0) == 0x00A0 || (inst & 0xFFF0) == 0x00B0) { // 0000 0000 101I xxxx
          // C
          // CLC
          // ClV
          // ClZ
          // ClN
          // CCC
          // S
          // SEC
          // SEV
          // SEZ
          // SEN
          // SCC
          //  15                    4   3  0
          // [0 0 0 0 0 0 0 0 1 0 1 I] [Mask]

          //DEBUG("Decoder: Condition code operator");

          pInst->type = _instt_condition_code;
          pInst->opcode = (_opcode)(inst & 0xFFF0);

          pInst->mask = inst & 0x000F;
        } else {
          // System instructions

          //DEBUG("Decoder: System instruction");

          pInst->type = _instt_system;
          pInst->opcode = (_opcode)inst;
        }
      } else {  // B000 0III xxxx xxxx; B != 0 && III != 0
        // Conditional branch instructions
        //  15      11  10   8   7    0
        // [x 0 0 0 0] [Opcode] [Offset]
        //
        // Opcode   Mnemonic
        // 0000xx   <system instructions>
        // 1000xx   BPL
        // 0004xx   BR
        // 1004xx   BMI
        // 0010xx   BNE
        // 1010xx   BHI
        // 0014xx   BEQ
        // 101400   BLOS
        // 0020xx   BGE
        // 1020xx   BVC
        // 0024xx   BLT
        // 1024xx   BVS
        // 0030xx   BGT
        // 1030xx   BCC or BHIS
        // 0034xx   BLE
        // 1034xx   BCS or BLO

        //DEBUG("Decoder: Conditional branch instruction");

        pInst->type = _instt_branch;
        pInst->opcode = (_opcode)(inst & 0xFF00);
        pInst->offset = SIGN_EXTEND_BYTE(inst);
      }
    }
  } else if((inst & 0x7000) == 0x7000) {  // x111 xxxx xxxx xxxx
    if((inst & 0xF000) == 0x7000) {   // 0111 xxxx xxxx xxxx
      // Double-operand instructions with register source operand
      //  15 12  11   9   8      6   5  3   2      0
      // [0111] [Opcode] [Register] [Mode] [Dest/Src]
      //
      // Opcode   Mnemonic
      // 00       MUL
      // 01       DIV
      // 02       ASH
      // 03       ASHC
      // 04       XOR
      // 05       Floating point operations
      // 06       System instructions
      // 07       SOB

      //DEBUG("Decoder: Double-operand instruction with register source operand");

      pInst->type = _instt_double_op_reg_src;
      pInst->opcode = (_opcode)(inst & 0xFE00);
      pInst->reg = (inst & 0x01C0) >> 6;
      pInst->srcMode = (CpuAddressingMode)((inst & 0x0038) >> 3);
      pInst->src = (inst & 0x0007) >> 0;
      pInst->offset = -(((inst & 0x003F) >> 0));
    } else {  // 1111 xxxx xxxx xxxx
      // TODO: Floating point instructions

      DEBUG("FPP INSTRUCTION: 0%06o", inst);

      return false;
    }
  } else {  // xIII xxxx xxxx xxxx; III != 0 && III != 7
    // Double-operand instructions
    //  15  14   12  11 9   8    6   5  3   2         0
    // [B] [Opcode] [Mode] [Source] [Mode] [Destination]
    //
    // Opcode   Mnemonic
    // 01       MOV / MOVB
    // 02       CMP / CMPB
    // 03       BIT / BITB
    // 04       BIC / BICB
    // 05       BIS / BISB
    // 06       ADD / SUB

    //DEBUG("Decoder: Double-operand instruction");

    pInst->type = _instt_double_op;
    pInst->opcode = (_opcode)(inst & 0xF000);
    pInst->srcMode = (CpuAddressingMode)((inst & 0x0E00) >> 9);
    pInst->src = (inst & 0x01C0) >> 6;
    pInst->dstMode = (CpuAddressingMode)((inst & 0x0038) >> 3);
    pInst->dst = (inst & 0x0007) >> 0;
  }

  // TODO: Debug
  if(pInst->opcode == 0xFFFFFF) {
    DEBUG("UNKNOWN INSTRUCTION: 0%06o", inst);
    assert(false);
  }

  return true;
}

void CPU::Reset() {
}

cpu_word CPU::Read(un_addr addr) {
  assert(!(addr & 1));

  switch(addr) {
    case 0777776:   // PS
      // DEBUG("CPU: PS RD");
      return _PSW;

    case 0777570:   // Console Switch & Display Register
      return 0;

      // case 0777764:   // System I/D Register
      //     return 1;
  }

  assert(false);
  return 0;
}

void CPU::Write(un_addr addr, const PartialValue& data) {
  assert((addr & 1) == 0);

  switch(addr) {
    default:
      assert(false);
      break;

    case 0777776:   // PS
      // DEBUG("CPU: PS WR");
      _setPSW(data.GetValue(_PSW));
      break;

    case 0777570:   // Console Switch & Display Register
      // DEBUG("DISPLAY: 0x%04X", data);
      break;

      // case 0777764:   // System I/D Register
      //   break;
  }
}
