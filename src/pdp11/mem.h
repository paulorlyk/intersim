//
// Created by palulukan on 1/9/26.
//

#ifndef MEM_H_522A593C810E424585CF34CF9200CEB3
#define MEM_H_522A593C810E424585CF34CF9200CEB3

#include <memory>

#include "unibus.h"
#include "mmu.h"

class Mem {
  public:
    data_status_t Read(cpu_addr addr, cpu_space s, cpu_mode m, MMU* mmu);
    data_status_t Write(cpu_addr addr, cpu_space s, cpu_mode m, bool bByte, cpu_word data, MMU* mmu);

    void Reset();

    std::shared_ptr<Unibus> GetUnibus() { return _unibus; };
    std::shared_ptr<RAM> GetRAM() { return _ram; };

  private:
    std::shared_ptr<RAM> _ram = std::make_shared<RAM>();
    std::shared_ptr<Unibus> _unibus = std::make_shared<Unibus>(_ram);
};

#endif //MEM_H_522A593C810E424585CF34CF9200CEB3
