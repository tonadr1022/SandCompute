#include "Window.hpp"

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
};

}  // namespace sand
