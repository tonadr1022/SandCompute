#include "App.hpp"

#include <SDL_timer.h>
#include <imgui.h>

#include <cstddef>

#include "Input.hpp"
#include "Path.hpp"
#include "gl/Buffer.hpp"
#include "gl/OpenGLDebug.hpp"
#include "gl/ShaderManager.hpp"
#include "gl/VertexArray.hpp"

namespace sand {
namespace {
constexpr size_t kDefaultScreenWidth{1600};
constexpr size_t kDefaultScreenHeight{900};
}  // namespace

struct Vertex {
  float x, y, z;
  float u, v;
};

constexpr const std::array<Vertex, 4> kQuadVertices = {{{-1, 1, 0.0f, 0.0f, 1.0f},
                                                        {-1, -1, 0.0f, 0.0f, 0.0f},
                                                        {1, 1, 0.0f, 1.0f, 1.0f},
                                                        {1, -1, 0.0f, 1.0f, 0.0f}

}};
// constexpr const std::array<uint32_t, 6> kQuadIndices = {0, 1, 2, 2, 1, 3};
App::App()
    : window_(kDefaultScreenWidth, kDefaultScreenHeight, "Sand Sim",
              [this](SDL_Event& event) { OnEvent(event); }) {}

void App::Run() {
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(MessageCallback, nullptr);
  ShaderManager::Init();
  ShaderManager::Get().AddShader("demo", {{GET_SHADER_PATH("demo.cs.glsl"), ShaderType::kCompute}});
  ShaderManager::Get().AddShader("quad",
                                 {{GET_SHADER_PATH("quad.vs.glsl"), ShaderType::kVertex},
                                  {GET_SHADER_PATH("quad.fs.glsl"), ShaderType::kFragment}});

  constexpr size_t kBoardX{kDefaultScreenWidth};
  constexpr size_t kBoardY{kDefaultScreenHeight};
  VertexArray quad_vao;
  Buffer quad_vbo;
  quad_vao.Init();
  quad_vao.EnableAttribute<float>(0, 3, offsetof(Vertex, x));
  quad_vao.EnableAttribute<float>(1, 2, offsetof(Vertex, u));
  quad_vbo.Init(sizeof(kQuadVertices), 0, kQuadVertices.data());
  quad_vao.AttachVertexBuffer(quad_vbo.Id(), 0, 0, sizeof(Vertex));

  uint32_t data_tex;
  glCreateTextures(GL_TEXTURE_2D, 1, &data_tex);
  glTextureStorage2D(data_tex, 1, GL_RGBA32F, kBoardX, kBoardY);
  glTextureParameteri(data_tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(data_tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(data_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(data_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  float w = 400, h = 400;
  std::vector<float> pixels(static_cast<size_t>(w * h * 4));
  for (int i = 0; i < w * h * 4; i++) {
    pixels[i] = 1.0;
  }
  glTextureSubImage2D(data_tex, 0, 100, 100, w, h, GL_RGBA, GL_FLOAT, pixels.data());
  glBindImageTexture(0, data_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

  double curr_time = SDL_GetPerformanceCounter();
  double prev_time = curr_time;
  window_.SetVsync(false);
  while (!window_.ShouldClose()) {
    curr_time = SDL_GetPerformanceCounter();
    double dt = ((curr_time - prev_time) / static_cast<double>(SDL_GetPerformanceFrequency()));
    prev_time = curr_time;
    static double sum = 0;
    static double count = 0;
    sum += dt;
    count++;
    if (static_cast<size_t>(curr_time) % 10 == 0) {
      window_.SetTitle("Frame Time:" + std::to_string(sum / count) +
                       ", FPS: " + std::to_string(count / sum));
      count = 0;
      sum = 0;
    }
    window_.PollEvents();
    window_.StartRenderFrame(imgui_enabled_);

    Shader compute_shader = ShaderManager::Get().GetShader("demo").value();
    compute_shader.Bind();
    glDispatchCompute(kBoardX, kBoardY, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    Shader quad_shader = ShaderManager::Get().GetShader("quad").value();
    quad_shader.Bind();
    quad_vao.Bind();
    glBindTexture(GL_TEXTURE_2D, data_tex);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (imgui_enabled_) OnImGui();
    window_.EndRenderFrame(imgui_enabled_);
  }

  glDeleteTextures(1, &data_tex);
  ShaderManager::Shutdown();
}

void App::OnEvent(const SDL_Event& event) {
  if (event.type == SDL_KEYDOWN) {
    if (event.key.keysym.sym == SDLK_g && event.key.keysym.mod & KMOD_ALT) {
      imgui_enabled_ = !imgui_enabled_;
      return;
    }
    if (event.key.keysym.sym == SDLK_r && event.key.keysym.mod & KMOD_ALT) {
      ShaderManager::Get().RecompileShaders();
    }
  }

  switch (event.type) {
    case SDL_KEYDOWN:
      Input::SetKeyPressed(event.key.keysym.sym, true);
      break;
    case SDL_KEYUP:
      Input::SetKeyPressed(event.key.keysym.sym, false);
      break;
    case SDL_MOUSEBUTTONDOWN:
      Input::SetMouseButtonPressed(event.button.button, true);
      break;
    case SDL_MOUSEBUTTONUP:
      Input::SetMouseButtonPressed(event.button.button, false);
      break;
  }
}
void App::OnImGui() {
  ImGui::Begin("Sand");
  ImGui::Text("test");
  ImGui::End();
}

}  // namespace sand
