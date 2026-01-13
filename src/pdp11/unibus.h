//
// Created by palulukan on 1/11/26.
//

#ifndef UNIBUS_H_60426E3DE2944C94A6BCA44305C940B3
#define UNIBUS_H_60426E3DE2944C94A6BCA44305C940B3

#include <cstdint>
#include <vector>
#include <memory>
#include <optional>

// 22-bit physical address
typedef uint32_t ph_size;
typedef uint32_t ph_addr;

// 18-bit Unibus address
typedef uint32_t un_size;
typedef uint32_t un_addr;

// 16-bit CPU virtual address
typedef uint16_t cpu_word;
typedef uint16_t cpu_addr;

#define MEM_UNIBUS_PERIPH_PAGE_ADDR 0x3E000
#define MEM_UNIBUS_ADDR_MAX         0x3FFFF

#define MEM_WORD_SIZE   2

#define MEM_PERIPH_PAGE_SIZE_WORDS ((MEM_UNIBUS_ADDR_MAX - MEM_UNIBUS_PERIPH_PAGE_ADDR + 1) / MEM_WORD_SIZE)

#define MEM_16BIT_PERIPH_PAGE_ADDR     0xE000
#define MEM_18BIT_PERIPH_PAGE_ADDR     MEM_UNIBUS_PERIPH_PAGE_ADDR
#define MEM_22BIT_UNIBUS_ADDR          0x3C0000

#define MEM_SIZE_WORDS  (MEM_22BIT_UNIBUS_ADDR / MEM_WORD_SIZE)
#define MEM_SIZE_BYTES  (MEM_SIZE_WORDS * MEM_WORD_SIZE)

#define IRQ_PRIORITY_MAX 7

#define MEM_HAS_ERR         (1 << 31)
#define MEM_ERR_MMU_ABORTED (1 << 30)
#define MEM_ERR_ILL_HLT     (1 << 7)
#define MEM_ERR_ODD_ADDR    (1 << 6)
#define MEM_ERR_NX_MEM      (1 << 5)
#define MEM_ERR_UNB_TIMEOUT (1 << 4)
#define MEM_ERR_YZ_STACK    (1 << 3)
#define MEM_ERR_RZ_STACK    (1 << 2)

struct DataStatus {
  DataStatus(cpu_word data, bool isError = false):
    data(data),
    error(isError)
  {}

  static auto Error(cpu_word code) { return DataStatus(code, true); }

  cpu_word data;
  bool error;
};

class PartialValue {
  public:
    explicit PartialValue(cpu_word data, cpu_word mask = 0xFFFF):
      _data(data),
      _mask(mask)
    {}

    cpu_word GetValue(cpu_word curValue) const { return (_data & _mask) | (curValue & ~_mask); }

  private:
    cpu_word _data;
    cpu_word _mask;
};

class RAM {
  public:
    void Poke(ph_addr base, const uint8_t* buf, ph_size size);

    bool LoadFile(const char* imageFileName, ph_addr base);
    std::optional<cpu_addr> LoadTape(const char* imageFileName, ph_addr base);

    bool DumpToFile(const char* fileName);

    DataStatus Read(ph_addr addr);
    DataStatus Write(ph_addr addr, const PartialValue& data);

  private:
    ssize_t _readFile(const char* imageFileName, uint8_t* buf, size_t size);

  private:
    cpu_word _mem[MEM_SIZE_WORDS] = {};
};

struct IoWindow {
  un_addr start;
  un_size size;
};

class UnibusDevice {
  friend class Unibus;

  public:
    UnibusDevice(const std::vector<IoWindow>& ioMap, int irqPriority, const char* name):
      _ioMap(ioMap),
      _irqPriority(irqPriority),
      _name(name)
    {}
    virtual ~UnibusDevice() = default;

    const char* GetName() const { return _name; }

  protected:
    virtual void Reset() = 0;
    virtual cpu_word Read(un_addr addr) = 0;
    virtual void Write(un_addr addr, const PartialValue& data) = 0;
    virtual cpu_word IrqAck() = 0;

  protected:
    std::vector<IoWindow> _ioMap;
    int _irqPriority;
    bool _bIRQ = false;
    const char* _name;
};

class Unibus {
  public:
    explicit Unibus(const std::shared_ptr<RAM>& ram):
      _ram(ram)
    {}

    DataStatus Read(un_addr addr);
    DataStatus Write(un_addr addr, const PartialValue& data);

    void RegisterDevice(UnibusDevice* dev);
    void UnregisterDevice(UnibusDevice* dev);

    void NotifyIRQ(UnibusDevice* dev);

    UnibusDevice* GetIRQ(int minPriority) const;
    cpu_word AckIRQ();
    bool HasIRG() const { return _currIRQDevice != nullptr; }

    void Reset();

  private:
    UnibusDevice* _getIoHandler(un_addr addr) const;
    void _updateCurrIRQDevice();

  private:
    UnibusDevice* _peripheralPageMap[MEM_PERIPH_PAGE_SIZE_WORDS] = {};
    std::vector<UnibusDevice *> _devices = {};
    UnibusDevice *_currIRQDevice = nullptr;
    std::shared_ptr<RAM> _ram;
};

#endif //UNIBUS_H_60426E3DE2944C94A6BCA44305C940B3
