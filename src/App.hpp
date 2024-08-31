#include "Window.hpp"
#include "sand_sim/SandSim.hpp"

namespace sand {

class App {
 public:
  App();
  void Run();

 private:
  Window window_;
  bool imgui_enabled_{true};
  void OnEvent(const SDL_Event& event);
  void OnImGui();
  static constexpr const uint32_t kWorkGroupX = 10, kWorkGroupY = 10;
  SandSim sand_sim_;
};

}  // namespace sand
