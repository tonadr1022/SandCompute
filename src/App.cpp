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
#include "gl/VertexArray.hpp"
#include "pch.hpp"

namespace sand {
namespace {
constexpr size_t kDefaultScreenWidth{1600};
constexpr size_t kDefaultScreenHeight{900};
}  // namespace

struct Vertex {
  float x, y, z;
  float u, v;
};

enum class MaterialType : uint8_t { kNone = 0, kSand = 1, kWater = 2 };

struct CellData {
  MaterialType material_type : 4;
  uint8_t color_index : 4;
  [[nodiscard]] uint32_t Pack() const { return Pack(material_type, color_index); }
  static uint32_t Pack(MaterialType material_type, uint8_t color_index) {
    return static_cast<uint8_t>(material_type) | color_index << 4;
    // return static_cast<uint8_t>(material_type) << 4 | color_index;
  }
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

  auto load_tex = []() -> uint32_t {
    uint32_t data_tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &data_tex);
    glTextureStorage2D(data_tex, 1, GL_R32UI, kBoardX, kBoardY);
    glTextureParameteri(data_tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(data_tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(data_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(data_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return data_tex;
  };

  double curr_time = SDL_GetPerformanceCounter();
  double prev_time = curr_time;
  window_.SetVsync(false);
  uint32_t curr_tex = load_tex();
  uint32_t prev_tex = load_tex();
  int height = 1;
  std::vector<uint32_t> data;
  std::vector<uint32_t> data2;
  data.reserve(kBoardX * kBoardY);
  data2.reserve(kBoardX * kBoardY);
  for (size_t i = 0; i < kBoardX * (kBoardY - height - 1); i++) {
    data.emplace_back(CellData::Pack(MaterialType::kNone, 0));
    data2.emplace_back(CellData::Pack(MaterialType::kNone, 0));
  }
  for (size_t i = 0; i < kBoardX * height; i++) {
    data.emplace_back(CellData::Pack(MaterialType::kSand, 0));
    data2.emplace_back(CellData::Pack(MaterialType::kNone, 0));
  }
  for (size_t i = 0; i < kBoardX * height; i++) {
    data.emplace_back(CellData::Pack(MaterialType::kNone, 0));
    data2.emplace_back(CellData::Pack(MaterialType::kNone, 0));
  }
  glTextureSubImage2D(curr_tex, 0, 0, 0, kBoardX, kBoardY, GL_RED_INTEGER, GL_UNSIGNED_INT,
                      data2.data());
  glTextureSubImage2D(prev_tex, 0, 0, 0, kBoardX, kBoardY, GL_RED_INTEGER, GL_UNSIGNED_INT,
                      data.data());

  while (!window_.ShouldClose()) {
    curr_time = SDL_GetPerformanceCounter();
    double dt = ((curr_time - prev_time) / static_cast<double>(SDL_GetPerformanceFrequency()));
    prev_time = curr_time;
    static double sum = 0;
    static uint32_t frame_count = 0;
    frame_count = (frame_count + 1) % UINT32_MAX;
    static int frame_counter_count = 0;
    sum += dt;
    frame_counter_count++;
    if (frame_counter_count % 1000 == 0) {
      window_.SetTitle("Frame Time:" + std::to_string(sum / frame_counter_count) +
                       ", FPS: " + std::to_string(frame_counter_count / sum));
      frame_counter_count = 0;
      sum = 0;
    }
    window_.PollEvents();
    window_.StartRenderFrame(imgui_enabled_);

    bool tick = (frame_count % 1000) == 0;
    if (tick) {
      Shader compute_shader = ShaderManager::Get().GetShader("demo").value();
      compute_shader.Bind();
      compute_shader.SetInt("grid_size_x", kBoardX);
      compute_shader.SetInt("grid_size_y", kBoardY);
      glBindImageTexture(0, prev_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
      glBindImageTexture(1, curr_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
      glDispatchCompute((kBoardX + kWorkGroupX - 1) / kWorkGroupX,
                        (kBoardY + kWorkGroupY - 1) / kWorkGroupY, 1);
      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
      std::swap(curr_tex, prev_tex);
    }

    Shader quad_shader = ShaderManager::Get().GetShader("quad").value();
    quad_shader.Bind();
    quad_vao.Bind();
    glBindTextureUnit(0, curr_tex);
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
