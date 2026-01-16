//
// Created by palulukan on 1/10/26.
//

#ifndef DL11_WINDOW_H_83E9D385E1AA46B98E35585D1083172F
#define DL11_WINDOW_H_83E9D385E1AA46B98E35585D1083172F

#include "../kw11.h"

#include "imgui.h"

#include <format>

class KW11_window {
  public:
    explicit KW11_window(const KW11* kw11):
      _kw11(kw11)
    {}

    void Render() {
      if(!show)
        return;

      ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, { 0.5f, 0.5f });
      ImGui::Begin("KW11-L - Line time clock", &show);

      auto sr = _kw11->Get_SR();
      if(ImGui::CollapsingHeader(std::format("SR: {:06o}", sr).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        ImGui::Text("IE = %d", (sr & KW11_SR_IE) != 0);
        ImGui::Text("IM = %d", (sr & KW11_SR_IM) != 0);

        ImGui::Unindent();
      }

      ImGui::End();
    }

  public:
    bool show = true;

  private:
    const KW11 *_kw11;
};

#endif //DL11_WINDOW_H_83E9D385E1AA46B98E35585D1083172F
