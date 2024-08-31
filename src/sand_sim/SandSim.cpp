#include "SandSim.hpp"

#include <imgui.h>

#include "Input.hpp"
#include "Window.hpp"
#include "gl/Buffer.hpp"
#include "gl/ShaderManager.hpp"
#include "gl/Texture.hpp"
#include "pch.hpp"

namespace sand {

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
enum class ModificationShape : uint32_t { kCircle, kSquare };
struct Modification {
  int x, y;
  float radius{10};
  ModificationShape shape;
  int cell{1};
};

struct SandSimImpl {
  SandSimImpl(const glm::ivec2& dims, const glm::ivec2& work_group_size)
      : dims(dims), work_group_size(work_group_size) {}
  glm::ivec2 dims;
  glm::ivec2 work_group_size;
  gl::Texture curr_tex;
  gl::Texture prev_tex;

  std::vector<Modification> modifications;
  gl::Buffer mod_buffer;
  ModificationShape mod_shape{ModificationShape::kCircle};
  float mod_radius{10};
};

// defined here due to pimpl
SandSim::SandSim(const Window& window) : window_(window) {}
SandSim::~SandSim() = default;

void SandSim::Start(const glm::ivec2& dims, const glm::ivec2& work_group_size) {
  impl_ = std::make_unique<SandSimImpl>(dims, work_group_size);
  impl_->mod_buffer.Init(sizeof(Modification) * 10000, GL_DYNAMIC_STORAGE_BIT);
  gl::Tex2DCreateInfoEmpty params{.dims = {dims.x, dims.y},
                                  .wrap_s = GL_CLAMP_TO_EDGE,
                                  .wrap_t = GL_CLAMP_TO_EDGE,
                                  .internal_format = GL_R32UI,
                                  .min_filter = GL_LINEAR,
                                  .mag_filter = GL_LINEAR};
  impl_->curr_tex.Load(params);
  impl_->prev_tex.Load(params);
  int height = 1;
  std::vector<uint32_t> data;
  std::vector<uint32_t> data2;
  data.reserve(static_cast<uint32_t>(dims.x * dims.y));
  data2.reserve(static_cast<uint32_t>(dims.x * dims.y));
  for (int i = 0; i < dims.x * (dims.y - height - 1); i++) {
    data.emplace_back(CellData::Pack(MaterialType::kNone, 0));
    data2.emplace_back(CellData::Pack(MaterialType::kNone, 0));
  }
  for (int i = 0; i < dims.x * height; i++) {
    data.emplace_back(CellData::Pack(MaterialType::kSand, 0));
    data2.emplace_back(CellData::Pack(MaterialType::kNone, 0));
  }
  for (int i = 0; i < dims.x * height; i++) {
    data.emplace_back(CellData::Pack(MaterialType::kNone, 0));
    data2.emplace_back(CellData::Pack(MaterialType::kNone, 0));
  }
  glTextureSubImage2D(impl_->curr_tex.Id(), 0, 0, 0, dims.x, dims.y, GL_RED_INTEGER,
                      GL_UNSIGNED_INT, data2.data());
  glTextureSubImage2D(impl_->prev_tex.Id(), 0, 0, 0, dims.x, dims.y, GL_RED_INTEGER,
                      GL_UNSIGNED_INT, data.data());
}

void SandSim::Update() {
  if (Input::IsMouseButtonPressed(SDL_BUTTON_LEFT)) {
    auto pos = window_.GetMousePosition();
    auto win_dims = window_.GetWindowSize();
    pos.y = win_dims.y - pos.y;
    // screen 1600,900
    glm::ivec2 true_pos = pos * impl_->dims / win_dims;

    impl_->modifications.emplace_back(Modification{.x = true_pos.x,
                                                   .y = true_pos.y,
                                                   .radius = impl_->mod_radius,
                                                   .shape = impl_->mod_shape,
                                                   .cell = 1});
  }
}
void SandSim::Simulate() const {
  gl::Shader compute_shader = gl::ShaderManager::Get().GetShader("demo").value();
  compute_shader.Bind();
  if (!impl_->modifications.empty()) {
    impl_->mod_buffer.SubDataStart(sizeof(Modification) * impl_->modifications.size(),
                                   impl_->modifications.data());
  }
  compute_shader.SetInt("modification_count", impl_->modifications.size());
  impl_->modifications.clear();

  compute_shader.SetInt("grid_size_x", impl_->dims.x);
  compute_shader.SetInt("grid_size_y", impl_->dims.y);

  glBindImageTexture(0, impl_->prev_tex.Id(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
  glBindImageTexture(1, impl_->curr_tex.Id(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
  impl_->mod_buffer.BindBase(GL_SHADER_STORAGE_BUFFER, 0);
  glDispatchCompute((impl_->dims.x + impl_->work_group_size.x - 1) / impl_->work_group_size.x,
                    (impl_->dims.y + impl_->work_group_size.y - 1) / impl_->work_group_size.y, 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  std::swap(impl_->curr_tex, impl_->prev_tex);
}

const gl::Texture& SandSim::GetCurrTex() const { return impl_->curr_tex; }

bool SandSim::OnEvent(const SDL_Event& event) {
  return false;
  if (event.type == SDL_MOUSEBUTTONDOWN) {
    auto pos = window_.GetMousePosition();
    auto win_dims = window_.GetWindowSize();
    pos.y = win_dims.y - pos.y;
    // screen 1600,900
    glm::ivec2 true_pos = pos * impl_->dims / win_dims;

    impl_->modifications.emplace_back(Modification{.x = true_pos.x,
                                                   .y = true_pos.y,
                                                   .radius = impl_->mod_radius,
                                                   .shape = impl_->mod_shape,
                                                   .cell = 1});
    return true;
  }
  return false;
}

void SandSim::OnImGui() {
  ImGui::Begin("Sand");

  ImGui::End();
}
}  // namespace sand
