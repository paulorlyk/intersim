#include "log.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL.h>

#include "taskScheduler.h"

#include "pdp11/mem.h"

#include "pdp11/ui/cpu1170_window.h"
#include "pdp11/ui/mmu_window.h"
#include "pdp11/ui/kw11_window.h"
#include "pdp11/ui/dl11_window.h"
#include "pdp11/ui/rk11_window.h"

/*
RK02/03/05 Disk Unit 0

Loc.	Cont.	Instruction	Comment
=======================================
001000	012700	mov #rkwc, r0	controller address
001002	177406
001004	012710	mov #-256,(r0)	set the word count
001006	177400
001010	012740	mov #5,-(r0)	read command
001012	000005
001014	105710	tstb (r0)	wait for ready
001016	100376  bpl .-2
001020	005007	clr pc		start loaded bootstrap with jump to 0
*/
static const cpu_word _bootstrap_RK11[] = {
  0012700,    // mov #rkwc, r0
  0177406,
  0012710,    // mov #-256,(r0)
  0177400,
  0012740,    // mov #5,-(r0)
  0000005,
  0105710,    // tstb (r0)
  0100376,    // bpl .-2
  0005007     // clr pc
};
const cpu_addr _bootstrapBase = 0001000;

int main() {
  // Setup SDL
  if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    ERROR("SDL_Init(): %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  // Create window with SDL_Renderer graphics context
  const float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  SDL_Window* window = SDL_CreateWindow(
    "intersim - Interactive Simulator",
    (int)(1280 * main_scale),
    (int)(800 * main_scale),
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY
  );
  if(!window) {
    ERROR("SDL_CreateWindow(): %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
  if(!renderer) {
    ERROR("SDL_CreateRenderer(): %s", SDL_GetError());
    return EXIT_FAILURE;
  }
  SDL_SetRenderVSync(renderer, 1);

  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // io.IniFilename = nullptr;
  io.LogFilename = nullptr;

  // Setup Dear ImGui style
  // ImGui::StyleColorsDark();
  ImGui::StyleColorsLight();
  // ImGui::StyleColorsClassic();

  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);
  style.FontScaleDpi = main_scale;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  // style.FontSizeBase = 14.0f;
  io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("third_party/imgui/imgui/misc/fonts/DroidSans.ttf");
  // io.Fonts->AddFontFromFileTTF("third_party/imgui/imgui/misc/fonts/Roboto-Medium.ttf");
  // io.Fonts->AddFontFromFileTTF("third_party/imgui/imgui/misc/fonts/Cousine-Regular.ttf");
  // io.Fonts->AddFontFromFileTTF("third_party/imgui/imgui/misc/fonts/Karla-Regular.ttf");

  auto scheduler = std::make_shared<TaskScheduler>();

  Mem mem;
  mem.GetRAM()->Poke(_bootstrapBase, (uint8_t *)_bootstrap, sizeof(_bootstrap));
  mem.GetRAM()->Poke(_bootstrapBase, (uint8_t *)_bootstrap_RK11, sizeof(_bootstrap_RK11));
  // if(!mem.GetRAM()->LoadTape("img/MAINDEC/MAINDEC-11-D0AA-PB.ptap", 0)) return EXIT_FAILURE;
  // if(!mem.GetRAM()->LoadTape("img/MAINDEC/MAINDEC-11-D0BA-PB.ptap", 0)) return EXIT_FAILURE;

  // if(!mem.GetRAM()->DumpToFile("img/ram.img")) return EXIT_FAILURE;

  CPU cpu(&mem, _bootstrapBase);
  // CPU cpu(&mem, 0200);

  RK11 rk11(mem.GetUnibus(), scheduler);
  KW11 kw11(mem.GetUnibus(), scheduler);
  DL11 dl11(mem.GetUnibus(), scheduler, 0777560, 060);

  if(!rk11.LoadDisk("img/unix_v5_rk/unix_v5_rk.dsk", 0))
  // if(!rk11.LoadDisk("img/xxdp/xxdp-rk.dsk", 0))
    return EXIT_FAILURE;

  CPU1170_window cpu1170_window(&cpu);
  MMU_window mmu_window(cpu.GetMMU());
  KW11_window kw11_window(&kw11);
  DL11_window dl11_window(&dl11);
  RK11_window rk11_window(&rk11);

  bool done = false;
  auto doRender = [&]() {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);

      if(event.type == SDL_EVENT_QUIT)
        done = true;

      if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
        done = true;
    }

    if(SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
      return;

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    {
      ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_FirstUseEver);
      ImGui::Begin("Devices");

      ImGui::Checkbox("PDP-11/70 CPU", &cpu1170_window.show);
      ImGui::Checkbox("MMU", &mmu_window.show);
      ImGui::Checkbox("KW11", &kw11_window.show);
      ImGui::Checkbox("DL11", &dl11_window.show);
      ImGui::Checkbox("RK11", &rk11_window.show);

      ImGui::End();
    }

    cpu1170_window.Render();
    mmu_window.Render();
    kw11_window.Render();
    dl11_window.Render();
    rk11_window.Render();

    // ImGui::ShowDemoWindow();

    // Rendering
    ImGui::Render();

    SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

    // SDL_SetRenderDrawColorFloat(renderer, 0.45f, 0.55f, 0.60f, 1.00f);
    SDL_SetRenderDrawColorFloat(renderer, 0.4f, 0.4f, 0.4f, 1.00f);
    SDL_RenderClear(renderer);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

    SDL_RenderPresent(renderer);
  };

  auto tsRender = scheduler->SetInterval(doRender, TS_SECONDS / 60);

  auto tsCpu = scheduler->SetInterval([&cpu]() {
    for(int i = 0; i < 300000; ++i) {
      if(!cpu.Run())
        break;
    }
  }, 1 * TS_MILLISECONDS);

  while(!done) {
    auto idleTime = scheduler->Run();

    SDL_DelayNS(idleTime);
  }

  scheduler->Cancel(tsCpu);
  tsCpu = TS_NULL_TASK;

  scheduler->Cancel(tsRender);
  tsRender = TS_NULL_TASK;

  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();

  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return EXIT_SUCCESS;
}