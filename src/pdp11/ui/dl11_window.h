//
// Created by palulukan on 1/10/26.
//

#ifndef DL11_WINDOW_H_B49499B837A64F9AB8B89DBCD53E0BE6
#define DL11_WINDOW_H_B49499B837A64F9AB8B89DBCD53E0BE6

#include "../dl11.h"

#include "imgui.h"
#include "imgui_stdlib.h"

#include <format>

class DL11_window {
  public:
    explicit DL11_window(DL11* dl11):
      _dl11(dl11)
    {
      _dl11->SetOnTx([this](char ch) {
        _onRecv(ch);
      });
    }

    void Render() {
      if(!show)
        return;

      ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, { 0.5f, 0.5f });
      ImGui::Begin("DL11 - Asynchronous line interface", &show);

      ImGui::Text("Addr: %06o", _dl11->GetBaseAddress());
      ImGui::SameLine();
      ImGui::Text("IRQ : 0%o", _dl11->GetBaseVector());

      auto rcsr = _dl11->Get_RCSR();
      if(ImGui::CollapsingHeader(std::format("RCSR: {:06o}", rcsr).c_str())) {
        ImGui::Indent();

        ImGui::Text("RDR_ENB = %d", (rcsr & DL11_RCSR_RDR_ENB) != 0);
        ImGui::Text("DTR = %d", (rcsr & DL11_RCSR_DTR) != 0);
        ImGui::Text("REQ_TO_SEND = %d", (rcsr & DL11_RCSR_REQ_TO_SEND) != 0);
        ImGui::Text("SEC_XMIT = %d", (rcsr & DL11_RCSR_SEC_XMIT) != 0);
        ImGui::Text("DATASET_INT_ENB = %d", (rcsr & DL11_RCSR_DATASET_INT_ENB) != 0);
        ImGui::Text("RCVR_INT_ENB = %d", (rcsr & DL11_RCSR_RCVR_INT_ENB) != 0);
        ImGui::Text("RCVR_DONE = %d", (rcsr & DL11_RCSR_RCVR_DONE) != 0);
        ImGui::Text("SEC_REC = %d", (rcsr & DL11_RCSR_SEC_REC) != 0);
        ImGui::Text("RCVR_ACT = %d", (rcsr & DL11_RCSR_RCVR_ACT) != 0);
        ImGui::Text("CAR_DET = %d", (rcsr & DL11_RCSR_CAR_DET) != 0);
        ImGui::Text("CLR_TO_SEND = %d", (rcsr & DL11_RCSR_CLR_TO_SEND) != 0);
        ImGui::Text("CLR_TO_SEND = %d", (rcsr & DL11_RCSR_CLR_TO_SEND) != 0);
        ImGui::Text("RING = %d", (rcsr & DL11_RCSR_RING) != 0);
        ImGui::Text("DATASET_INT = %d", (rcsr & DL11_RCSR_DATASET_INT) != 0);

        ImGui::Unindent();
      }

      {
        auto rbuf = _dl11->Get_RBUF();
        const char ch = DL11_RBUF_GET_DATA(rbuf);
        auto chStr = _printCharacter(ch);

        if(ImGui::CollapsingHeader(std::format("RBUF: {:06o} - '{}'", rbuf, chStr).c_str())) {
          ImGui::Indent();

          ImGui::Text("DATA = 0%03o (%s)", ch, chStr);
          ImGui::Text("P_ERR = %d", (rbuf & DL11_RBUF_P_ERR) != 0);
          ImGui::Text("FR_ERR = %d", (rbuf & DL11_RBUF_FR_ERR) != 0);
          ImGui::Text("OR_ERR = %d", (rbuf & DL11_RBUF_OR_ERR) != 0);
          ImGui::Text("ERROR = %d", (rbuf & DL11_RBUF_ERROR) != 0);

          ImGui::Unindent();
        }
      }

      auto xcsr = _dl11->Get_XCSR();
      if(ImGui::CollapsingHeader(std::format("XCSR: {:06o}", xcsr).c_str())) {
        ImGui::Indent();

        ImGui::Text("BREAK = %d", (xcsr & DL11_XCSR_BREAK) != 0);
        ImGui::Text("MAINT = %d", (xcsr & DL11_XCSR_MAINT) != 0);
        ImGui::Text("XMIT_INT_ENB = %d", (xcsr & DL11_XCSR_XMIT_INT_ENB) != 0);
        ImGui::Text("XMIT_RDY = %d", (xcsr & DL11_XCSR_XMIT_RDY) != 0);

        ImGui::Unindent();
      }

      {
        auto xbuf = _dl11->Get_XBUF();
        const char ch = DL11_XBUF_GET_DATA(xbuf);
        auto chStr = _printCharacter(ch);

        if(ImGui::CollapsingHeader(std::format("XBUF: {:06o} - '{}'", xbuf, chStr).c_str())) {
          ImGui::Indent();

          ImGui::Text("DATA = 0%03o (%s)", ch, chStr);

          ImGui::Unindent();
        }
      }

      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_InputTextCursor, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

      ImGui::InputTextMultiline(
        "##term_screen",
        &_screenBuf,
        ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 14),
        ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_CallbackAlways,
        _inputResizeCallback,
        this
      );

      ImGui::PopStyleColor(3);

      ImGui::End();
    }

  private:
    static int _inputResizeCallback(ImGuiInputTextCallbackData* data) {
      auto pThis = (DL11_window *)data->UserData;

      if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        if(data->EventChar)
          pThis->_onKeyPress((char)data->EventChar);

        // Discard change
        data->EventChar = 0;
      } else if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
        if(ImGui::IsKeyDown(ImGuiKey_Backspace))
          pThis->_onKeyPress('\b');

        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, pThis->_buf.c_str());
      }

      return 0;
    }

    void _onKeyPress(char ch) {
      _dl11->Receive(ch);
    }

    void _onRecv(char ch) {
      ch &= 0x7F;

      if(ch == '\b') {
        if(!_buf.empty())
          _buf.pop_back();
      } else if(isspace(ch)) {
        _buf += ch;
      } else if(ch != '\0') {
        _buf += _printCharacter(ch);
      }

      _screenBuf = _buf;
    }

    const char* _printCharacter(char ch) {
      ch &= 0x7F;

      return _chMap[(unsigned char)ch];
    }

  public:
    bool show = true;

  private:
    DL11 *_dl11;
    std::string _buf;
    std::string _screenBuf;

  private:
    const char* const _chMap[256] = {
      "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI", "DLE",
      "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC","FS", "GS", "RS", "US", "SPACE",
      "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
      "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
      ":", ";", "<", "=", ">", "?", "@",
      "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
      "[", "\\", "]", "^", "_", "`",
      "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
      "{", "|", "}", "~", "DEL",
      "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
      "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
      "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
      "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
      "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?",
      "?", "?", "?",
    };
};

#endif //DL11_WINDOW_H_B49499B837A64F9AB8B89DBCD53E0BE6
