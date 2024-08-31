#include "App.hpp"

#include <SDL_timer.h>
#include <imgui.h>

#include <cstddef>
#include <string>

#include "Input.hpp"
#include "Path.hpp"
#include "gl/Buffer.hpp"
#include "gl/OpenGLDebug.hpp"
#include "gl/ShaderManager.hpp"
#include "gl/Texture.hpp"
#include "gl/VertexArray.hpp"
#include "pch.hpp"
#include "sand_sim/SandSim.hpp"

using gl::Buffer;
using gl::Shader;
using gl::ShaderManager;
using gl::ShaderType;
using gl::VertexArray;

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
              [this](SDL_Event& event) { OnEvent(event); }),
      sand_sim_(window_) {}

void App::Run() {
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(gl::MessageCallback, nullptr);
  ShaderManager::Init();
  ShaderManager::Get().AddShader(
      "demo", {
                  {GET_SHADER_PATH("demo.cs.glsl"),
                   ShaderType::kCompute,
                   {std::make_pair("WORK_GROUP_X", std::to_string(kWorkGroupX)),
                    std::make_pair("WORK_GROUP_Y", std::to_string(kWorkGroupY))}},
              });

  ShaderManager::Get().AddShader("quad",
                                 {{GET_SHADER_PATH("quad.vs.glsl"), ShaderType::kVertex, {}},
                                  {GET_SHADER_PATH("quad.fs.glsl"), ShaderType::kFragment, {}}});

  constexpr size_t kBoardX{kDefaultScreenWidth};
  constexpr size_t kBoardY{kDefaultScreenHeight};
  VertexArray quad_vao;
  Buffer quad_vbo;
  quad_vao.Init();
  quad_vao.EnableAttribute<float>(0, 3, offsetof(Vertex, x));
  quad_vao.EnableAttribute<float>(1, 2, offsetof(Vertex, u));
  quad_vbo.Init(sizeof(kQuadVertices), 0, kQuadVertices.data());
  quad_vao.AttachVertexBuffer(quad_vbo.Id(), 0, 0, sizeof(Vertex));

  double curr_time = SDL_GetPerformanceCounter();
  double prev_time = curr_time;
  window_.SetVsync(true);

  sand_sim_.Start({kBoardX, kBoardY}, {kWorkGroupX, kWorkGroupY});

  while (!window_.ShouldClose()) {
    curr_time = SDL_GetPerformanceCounter();
    double dt = ((curr_time - prev_time) / static_cast<double>(SDL_GetPerformanceFrequency()));
    prev_time = curr_time;
    static double sum = 0;
    // static size_t frame_count = 0;
    // frame_count++;
    static int frame_counter_count = 0;
    sum += dt;
    frame_counter_count++;
    if (frame_counter_count % 100 == 0) {
      window_.SetTitle("Frame Time:" + std::to_string(sum / frame_counter_count) +
                       ", FPS: " + std::to_string(frame_counter_count / sum));
      frame_counter_count = 0;
      sum = 0;
    }
    window_.PollEvents();
    sand_sim_.Update();
    window_.StartRenderFrame(imgui_enabled_);

    static double s = 0;
    s += dt;
    bool tick = false;
    if (s > 1.f / 120.f) {
      s = 0;
      tick = true;
    }
    if (tick) {
      sand_sim_.Simulate();
    }

    Shader quad_shader = ShaderManager::Get().GetShader("quad").value();
    quad_shader.Bind();
    quad_vao.Bind();
    glBindTextureUnit(0, sand_sim_.GetCurrTex().Id());
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (imgui_enabled_) OnImGui();
    window_.EndRenderFrame(imgui_enabled_);
  }

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
      return;
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

  if (sand_sim_.OnEvent(event)) return;
}
void App::OnImGui() {
  ImGui::Begin("Sand");
  ImGui::Text("test");
  ImGui::End();
}

}  // namespace sand
