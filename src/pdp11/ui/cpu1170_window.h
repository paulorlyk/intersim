//
// Created by palulukan on 1/10/26.
//

#ifndef CPU1170_H_B49499B837A64F9AB8B89DBCD53E0BE6
#define CPU1170_H_B49499B837A64F9AB8B89DBCD53E0BE6

#include "../cpu.h"

#include "imgui.h"
#include "imgui_stdlib.h"

#include <format>

class CPU1170_window {
  public:
    explicit CPU1170_window(const CPU* cpu):
      _cpu(cpu)
    {}

    void Render() {
      if(!show)
        return;

      ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, { 0.5f, 0.5f });
      ImGui::Begin("PDP-11/70 CPU", &show);

      auto psw = _cpu->Get_PSW();
      if(ImGui::CollapsingHeader(std::format("PSW: {:04x}", psw).c_str())) {
        ImGui::Text("C = %d", PSW_GET_C(psw));
        ImGui::Text("V = %d", PSW_GET_V(psw));
        ImGui::Text("Z = %d", PSW_GET_Z(psw));
        ImGui::Text("N = %d", PSW_GET_N(psw));
        ImGui::Text("TRAP = %d", PSW_GET_TRAP(psw));
        ImGui::Text("PRIORITY = %d", PSW_GET_PRIORITY(psw));
        ImGui::Text("REG_SET = %d", PSW_GET_REG_SET(psw));
        ImGui::Text("PREV_MODE = %d", PSW_GET_PREV_MODE(psw));
        ImGui::Text("CUR_MODE = %d", PSW_GET_CUR_MODE(psw));
      }

      for(int rs = 0; rs < 2; ++rs) {
        ImGui::PushID(rs);

        ImGui::BulletText("Reg set %d%s", rs, PSW_GET_REG_SET(psw) == rs ? " - current" : "");

        ImGui::Indent();
        for(int i = 0; i < 8; ++i) {
          ImGui::PushID(i);

          ImGui::Text("%s: %04x", _registerNames[i], _cpu->Get_RegSet(rs)[i]);

          if(i < 7 && (i % 2) == 0) {
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
          }

          ImGui::PopID();
        }

        ImGui::Unindent();

        ImGui::PopID();
      }

      ImGui::BulletText("Last SP");
      ImGui::Indent();
      for(int i = 0; i < _cpu_mode_max; ++i) {
        ImGui::PushID(i);

        ImGui::Text("%s: %04x", _modeNames[i], _cpu->Get_LastSP()[i]);
        if(i < 3 && (i % 2) == 0) {
          ImGui::SameLine();
          ImGui::Spacing();
          ImGui::SameLine();
        }

        ImGui::PopID();
      }
      ImGui::Unindent();

      ImGui::RadioButton("Wait", _cpu->IsWait());
      ImGui::SameLine();
      ImGui::RadioButton("IRQ", _cpu->HasIRQ());

      ImGui::End();
    }

  public:
    bool show = true;

  private:
    const CPU *_cpu;

  private:
    const char * const _registerNames[8] = { "R0", "R1", "R2", "R3", "R4", "R5", "SP", "PC" };
    const char * const _modeNames[4] = { "KRN", "SVI", "INV", "USR" };
};

#endif //CPU1170_H_B49499B837A64F9AB8B89DBCD53E0BE6
