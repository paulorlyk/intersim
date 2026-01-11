//
// Created by palulukan on 1/11/26.
//

#ifndef RK11_WINDOW_H_8E4073A83F5149369295418665D23C8B
#define RK11_WINDOW_H_8E4073A83F5149369295418665D23C8B

#include "../rk11.h"

#include "imgui.h"

#include <format>

class RK11_window {
  public:
    explicit RK11_window(const RK11* rk11):
      _rk11(rk11)
    {}

    void Render() {
      if(!show)
        return;

      ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, { 0.5f, 0.5f });
      ImGui::Begin("RK11 - Moving head disk drive controller", &show);

      ImGui::Text("RKDS: %04x", _rk11->Get_RKDS());
      ImGui::SameLine();
      ImGui::Text("RKER: %04x", _rk11->Get_RKER());
      ImGui::SameLine();
      ImGui::Text("RKCS: %04x", _rk11->Get_RKCS());
      ImGui::SameLine();
      ImGui::Text("RKWC: %04x", _rk11->Get_RKWC());

      ImGui::Text("RKBA: %04x", _rk11->Get_RKBA());
      ImGui::SameLine();
      ImGui::Text("RKDA: %04x", _rk11->Get_RKDA());
      ImGui::SameLine();
      ImGui::Text("RKMR: %04x", _rk11->Get_RKMR());
      ImGui::SameLine();
      ImGui::Text("RKDB: %04x", _rk11->Get_RKDB());

      ImGui::RadioButton("IRQ", _rk11->Get_IRQ());

      if(ImGui::CollapsingHeader("Disks", ImGuiTreeNodeFlags_DefaultOpen)) {
        for(int i = 0; i < RK05_DISKS_MAX; ++i) {
          auto disk = _rk11->Get_RK05(i);

          if(!disk.connected)
            continue;

          ImGui::PushID(i);

          ImGui::Text("RK05 %d:", i);
          ImGui::SameLine();
          ImGui::RadioButton("RDY", !disk.img.empty());
          ImGui::SameLine();
          ImGui::RadioButton("BSY", !disk.IsIdle());
          ImGui::SameLine();
          ImGui::RadioButton("IRQ", disk.irq);
          ImGui::SameLine();
          ImGui::RadioButton("WP", disk.writeProtect);
          ImGui::SameLine();
          ImGui::Text("CYL: %d", disk.cylinder);

          ImGui::PopID();
        }
      }

      ImGui::End();
    }

  public:
    bool show = true;

  private:
    const RK11 *_rk11;
};

#endif //RK11_WINDOW_H_8E4073A83F5149369295418665D23C8B
