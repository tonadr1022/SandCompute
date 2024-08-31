#pragma once

#include <SDL_events.h>

namespace gl {
class Texture;
}
namespace sand {

class Window;
struct SandSimImpl;

class SandSim {
 public:
  explicit SandSim(const Window& window);
  // default for pimpl
  ~SandSim();
  void Start(const glm::ivec2& dims, const glm::ivec2& work_group_size);
  void Simulate() const;
  void Update();
  bool OnEvent(const SDL_Event& event);
  void OnImGui();
  [[nodiscard]] const gl::Texture& GetCurrTex() const;

  const Window& window_;
  // pimpl
  std::unique_ptr<SandSimImpl> impl_{nullptr};
};
}  // namespace sand
