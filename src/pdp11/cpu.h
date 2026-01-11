//
// Created by palulukan on 1/10/26.
//

#ifndef CPU_H_2F37D0FD1AD74CD2B34F9BF5B0C44AF9
#define CPU_H_2F37D0FD1AD74CD2B34F9BF5B0C44AF9

#include "unibus.h"
#include "mem.h"
#include "mmu.h"

// PDP-11/70 CPU

#define GET_BIT_FIELD(word, mask, offset) (((word) & (mask)) >> (offset))
#define SET_BIT_FIELD(word, val, mask, offset) (((word) & ~(mask)) | (((val) << (offset)) & (mask)))

#define SIGN_EXTEND_BYTE(b) (((b) & 0xFF) ^ ((~0xFF) * (((b) & 0x0080) != 0)))   // Assume two's complement integers

typedef uint32_t operand_addr;

#define OPERAND_TYPE_REG    (1U << 16)
#define OPERAND_TYPE_BYTE   (1U << 17)

enum CpuAddressingMode {
  _reg               = 0, // The operand is in Rn
  _reg_deferred      = 1, // Rn contains the address of the operand
  _auto_inc          = 2, // Rn contains the address of the operand, then increment Rn by 1 in byte command and by 2 in word command
  _auto_inc_deferred = 3, // Rn contains the address of the address of the operand, then increment Rn by 2
  _auto_dec          = 4, // Decrement Rn by 1 in byte command and by 2 in word command, then use it as the address of the operand
  _auto_dec_deferred = 5, // Decrement Rn by 2, then use it as the address of the address of the operand
  _index             = 6, // Rn + X is the address of the operand
  _index_deferred    = 7  // Rn + X is the address of the address of the operand
};

// Current processor mode
// Allowed values: 0, 1, 3
#define PSW_CUR_MODE_MASK 0xC000
#define PSW_CUR_MODE_OFFSET 14
#define PSW_GET_CUR_MODE(psw) ((cpu_mode)GET_BIT_FIELD((psw), PSW_CUR_MODE_MASK, PSW_CUR_MODE_OFFSET))
#define PSW_SET_CUR_MODE(psw, val) SET_BIT_FIELD((psw), (val), PSW_CUR_MODE_MASK, PSW_CUR_MODE_OFFSET)

// Previous processor mode
// Allowed values: 0, 1, 3
#define PSW_PREV_MODE_MASK 0x3000
#define PSW_PREV_MODE_OFFSET 12
#define PSW_GET_PREV_MODE(psw) ((cpu_mode)GET_BIT_FIELD((psw), PSW_PREV_MODE_MASK, PSW_PREV_MODE_OFFSET))
#define PSW_SET_PREV_MODE(psw, val) SET_BIT_FIELD((psw), (val), PSW_PREV_MODE_MASK, PSW_PREV_MODE_OFFSET)

#define CPU_MODE_KERNEL     0
#define CPU_MODE_SUPERVISOR 1
#define CPU_MODE_USER       3

// Processor general register set
// Allowed values: 0 - 1
#define PSW_REG_SET_MASK 0x0800
#define PSW_REG_SET_OFFSET 11
#define PSW_GET_REG_SET(psw) GET_BIT_FIELD((psw), PSW_REG_SET_MASK, PSW_REG_SET_OFFSET)
#define PSW_SET_REG_SET(psw, val) SET_BIT_FIELD((psw), (val), PSW_REG_SET_MASK, PSW_REG_SET_OFFSET)

// Processor interrupt priority
// Allowed values: 0 - 7
#define PSW_PRIORITY_MASK 0x00E0
#define PSW_PRIORITY_OFFSET 5
#define PSW_GET_PRIORITY(psw) GET_BIT_FIELD((psw), PSW_PRIORITY_MASK, PSW_PRIORITY_OFFSET)
#define PSW_SET_PRIORITY(psw, val) SET_BIT_FIELD((psw), (val), PSW_PRIORITY_MASK, PSW_PRIORITY_OFFSET)

// Trap flag
// Allowed values: 0, 1
#define PSW_TRAP_MASK 0x0010
#define PSW_TRAP_OFFSET 4
#define PSW_GET_TRAP(psw) GET_BIT_FIELD((psw), PSW_TRAP_MASK, PSW_TRAP_OFFSET)
#define PSW_SET_TRAP(psw, val) SET_BIT_FIELD((psw), (val), PSW_TRAP_MASK, PSW_TRAP_OFFSET)

// Negative flag
// Allowed values: 0, 1
#define PSW_N_MASK 0x0008
#define PSW_N_OFFSET 3
#define PSW_GET_N(psw) GET_BIT_FIELD((psw), PSW_N_MASK, PSW_N_OFFSET)
#define PSW_SET_N(psw, val) SET_BIT_FIELD((psw), (val), PSW_N_MASK, PSW_N_OFFSET)

// Zero flag
// Allowed values: 0, 1
#define PSW_Z_MASK 0x0004
#define PSW_Z_OFFSET 2
#define PSW_GET_Z(psw) GET_BIT_FIELD((psw), PSW_Z_MASK, PSW_Z_OFFSET)
#define PSW_SET_Z(psw, val) SET_BIT_FIELD((psw), (val), PSW_Z_MASK, PSW_Z_OFFSET)

// Overflow flag
// Allowed values: 0, 1
#define PSW_V_MASK 0x0002
#define PSW_V_OFFSET 1
#define PSW_GET_V(psw) GET_BIT_FIELD((psw), PSW_V_MASK, PSW_V_OFFSET)
#define PSW_SET_V(psw, val) SET_BIT_FIELD((psw), (val), PSW_V_MASK, PSW_V_OFFSET)

// Carry flag
// Allowed values: 0, 1
#define PSW_C_MASK 0x0001
#define PSW_C_OFFSET 0
#define PSW_GET_C(psw) GET_BIT_FIELD((psw), PSW_C_MASK, PSW_C_OFFSET)
#define PSW_SET_C(psw, val) SET_BIT_FIELD((psw), (val), PSW_C_MASK, PSW_C_OFFSET)

#define PSW_MASK 0xF8FF

#define TRAP_ERR                    04
#define TRAP_RESERVED_INSTRUCTION   010
#define TRAP_EMT                    030
#define TRAP_TRAP                   034
#define TRAP_MMU                    0250

class CPU : public UnibusDevice {
  public:
    CPU(Mem *mem, cpu_word R7);
    ~CPU() override;

    void Reset() override;
    cpu_word Read(un_addr addr) override;
    void Write(un_addr addr, cpu_word data) override;
    cpu_word IrqAck() override;

    bool Run();

    const MMU* GetMMU() const { return &_mmu; }

    cpu_word Get_PSW() const { return _PSW; }
    const cpu_word* Get_GPR() const { return _GPR; }
    const cpu_word* Get_RegSet(int n) const { return _regSet[n]; }
    const cpu_word* Get_RegSet1() const { return _regSet[1]; }
    const cpu_word* Get_LastSP() const { return _lastSP; }
    bool IsWait() const { return _wait; }
    bool HasIRQ() const { return _mem->GetUnibus()->HasIRG(); }

  private:
    void _trap(cpu_word vec);
    void _setPSW(cpu_word psw);
    bool _read(cpu_addr addr, bool bByte, cpu_space space, cpu_mode mode, cpu_word *data);
    bool _write(cpu_addr addr, bool bByte, cpu_word data, cpu_space space, cpu_mode mode);
    bool _makeOperandAddress(CpuAddressingMode mode, int reg, bool bByte, operand_addr *pAddr);
    bool _load(CpuAddressingMode mode, int reg, bool bByte, operand_addr* pAddr, cpu_word* data);
    bool _store(operand_addr addr, cpu_word data);
    bool _push(cpu_word w);
    bool _pop(cpu_word* w);
    void _setFlags(bool n, bool z, bool v, bool c);
    int _fetchPC(cpu_word* data);
    bool _decode(cpu_word inst, struct _instruction* pInst);

  private:
    Mem *_mem;
    MMU _mmu;

    cpu_word *_GPR = _regSet[0];
    cpu_word _PSW = 0;

    cpu_word _regSet[2][8] = {};
    cpu_word _lastSP[4] = {};

    bool _wait = false;

    bool _inTrap = false;

    bool _mmuTrap = false;

    // TODO: Debug
    bool _disassemblyOutput = false;
};

#endif //CPU_H_2F37D0FD1AD74CD2B34F9BF5B0C44AF9
