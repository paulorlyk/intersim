//
// Created by palulukan on 1/10/26.
//

#ifndef MMU_WINDOW_H_B49499B837A64F9AB8B89DBCD53E0BE6
#define MMU_WINDOW_H_B49499B837A64F9AB8B89DBCD53E0BE6

#include "../mmu.h"

#include "imgui.h"
#include "imgui_stdlib.h"

#include <format>

class MMU_window {
  public:
    explicit MMU_window(const MMU* mmu):
      _mmu(mmu)
    {}

    void Render() {
      if(!show)
        return;

      ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, { 0.5f, 0.5f });
      ImGui::Begin("PDP-11/70 MMU", &show);

      ImGui::Text("MMR0: %06o", _mmu->Get_MMR0());
      ImGui::SameLine();
      ImGui::Spacing();
      ImGui::SameLine();
      ImGui::Text("MMR1: %06o", _mmu->Get_MMR1());
      ImGui::SameLine();
      ImGui::Spacing();
      ImGui::SameLine();
      ImGui::Text("MMR2: %06o", _mmu->Get_MMR2());
      ImGui::SameLine();
      ImGui::Spacing();
      ImGui::SameLine();
      ImGui::Text("MMR3: %06o", _mmu->Get_MMR3());

      for(int mode = 0; mode < _cpu_mode_max; ++mode) {
        if(mode == cpu_mode_Invalid)
          continue;

        ImGui::PushID(mode);

        if(ImGui::CollapsingHeader(_modeNames[mode], ImGuiTreeNodeFlags_DefaultOpen)) {
          for(int space = 0; space < _cpu_space_max; ++space) {
            ImGui::PushID(space);

            ImGui::BulletText("%c%cAR[0-7]", _modeNames[mode][0], _spaceNames[space][0]);
            ImGui::Indent();
            for(int i = 0; i < 8; ++i) {
              ImGui::PushID(i);

              ImGui::Text("%06o", _mmu->Get_PAR(space, mode)[i]);

              if(i < 7) {
                ImGui::SameLine();
                ImGui::Spacing();
                ImGui::SameLine();
              }

              ImGui::PopID();
            }
            ImGui::Unindent();

            ImGui::BulletText("%c%cDR[0-7]", _modeNames[mode][0], _spaceNames[space][0]);
            ImGui::Indent();
            for(int i = 0; i < 8; ++i) {
              ImGui::PushID(i);

              ImGui::Text("%06o", _mmu->Get_PDR(space, mode)[i]);

              if(i < 7) {
                ImGui::SameLine();
                ImGui::Spacing();
                ImGui::SameLine();
              }

              ImGui::PopID();
            }
            ImGui::Unindent();

            ImGui::PopID();
          }
        }

        ImGui::PopID();
      }

      ImGui::End();
    }

  public:
    bool show = true;

  private:
    const MMU *_mmu;

  private:
    const char * const _modeNames[4] = { "KRN", "SVI", "INV", "USR" };
    const char * const _spaceNames[2] = { "Instruction", "Data" };
};

#endif //MMU_WINDOW_H_B49499B837A64F9AB8B89DBCD53E0BE6
