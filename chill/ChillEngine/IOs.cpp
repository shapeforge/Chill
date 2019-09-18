#include "IOs.h"

#include "NodeEditor.h"

#include "Processor.h"

#include "FileDialog.h"

namespace chill {

ProcessorOutput::~ProcessorOutput() {
  for (std::shared_ptr<ProcessorInput> input : m_links)
    Processor::disconnect(input);
  m_links.clear();
}

//-------------------------------------------------------

bool ProcessorOutput::draw() {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  float w_scale = window->FontWindowScale;

  float radius = (style.socket_radius + style.socket_border_width) * w_scale;
  float full_radius = (style.socket_radius + style.socket_border_width) * w_scale;

  ImVec2 pos = ImGui::GetCursorPos();
  ImVec2 text_size = ImGui::CalcTextSize(name());

  ImGui::SetCursorPosX(pos.x - text_size.x - style.ItemSpacing.x * w_scale - full_radius);
  ImGui::SetCursorPosY(pos.y - text_size.y/2 + full_radius);

  if (w_scale > 0.7F) {
    ImGui::Text("%s", name());
  }

  ImGui::SetCursorPos(ImVec2(pos.x - full_radius + style.processor_border_width * w_scale / 2, pos.y));

  setPosition(ImVec2(pos.x, pos.y + full_radius));

  ImGui::PushStyleColor(ImGuiCol_Text, color());
  ImGui::PushStyleColor(ImGuiCol_Button, color());
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color());
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, color() & 0xAAFFFFFF);

  ImGui::PushStyleColor(ImGuiCol_DragDropTarget, style.pipe_selected_color);

  int id = int(getUniqueID());
  ImGui::PushID(id);

  ImGui::PushStyleColor(ImGuiCol_Border, 0x00000000);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.socket_border_width * w_scale);

  if (ImGui::Button("", ImVec2(radius, radius) * 2)) {
    NodeEditor::Instance()->setSelectedOutput(owner()->output(name()));
  }
  ImGui::PopStyleVar();
  ImGui::PopStyleColor();

  if (ImGui::BeginDragDropSource()) {
    NodeEditor::Instance()->setSelectedOutput(owner()->output(name()));
    ImGui::SetDragDropPayload("_pipe_output", nullptr, 0, ImGuiCond_Once);
    ImGui::EndDragDropSource();
  }

  if (ImGui::BeginDragDropTarget()) {
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_pipe_input")){
      NodeEditor::Instance()->setSelectedOutput(owner()->output(name()));
    }
    ImGui::EndDragDropTarget();
  }
  ImGui::PopID();

  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();

  return false;
}

//-------------------------------------------------------

std::shared_ptr<ProcessorOutput> ProcessorOutput::create(const std::string& _name, IOType::IOType _type = IOType::UNDEF, bool _emitable = false) {
  std::shared_ptr<ProcessorOutput> output;
  switch (_type) {
  case IOType::BOOLEAN:
    output = std::shared_ptr<ProcessorOutput>(new BoolOutput());
    break;
  case IOType::INTEGER:
    output = std::shared_ptr<ProcessorOutput>(new IntOutput());
    break;
  case IOType::PATH:
    output = std::shared_ptr<ProcessorOutput>(new PathOutput());
    break;
  case IOType::SCALAR:
    output = std::shared_ptr<ProcessorOutput>(new ScalarOutput());
    break;
  case IOType::SHAPE:
    output = std::shared_ptr<ProcessorOutput>(new ShapeOutput());
    break;
  case IOType::STRING:
    output = std::shared_ptr<ProcessorOutput>(new StringOutput());
    break;
  case IOType::VEC3:
    output = std::shared_ptr<ProcessorOutput>(new Vec3Output());
    break;
  case IOType::VEC4:
    output = std::shared_ptr<ProcessorOutput>(new Vec4Output());
    break;
  case IOType::UNDEF:
    output = std::shared_ptr<ProcessorOutput>(new UndefOutput());
    break;
  default:
    output = std::shared_ptr<ProcessorOutput>(new UndefOutput());
    break;
  }
  output->setName(_name);
  output->setType(_type);
  output->setEmitable(_emitable);
  return output;
}

//-------------------------------------------------------

ProcessorInput::~ProcessorInput() {
  Processor::disconnect(m_link);
}

//-------------------------------------------------------

bool ProcessorInput::draw() {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  float w_scale = window->FontWindowScale;

  bool updt = false;

  float radius = (style.socket_radius + style.socket_border_width) * w_scale;

  ImVec2 pos = ImGui::GetCursorPos();

    
    ImGui::SetCursorPos(ImVec2(pos.x - radius - style.processor_border_width * w_scale / 2, pos.y));
    setPosition(ImVec2(pos.x, pos.y + radius));

    if (!m_isDataOnly) {
        ImGui::PushStyleColor(ImGuiCol_Text, color());
        ImGui::PushStyleColor(ImGuiCol_Button, color());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color());
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color() & 0xAAFFFFFF);
        ImGui::PushStyleColor(ImGuiCol_DragDropTarget, style.pipe_selected_color);
    }
    else {
      ImGui::PushStyleColor(ImGuiCol_Text, 0X00000000);
      ImGui::PushStyleColor(ImGuiCol_Button, 0X00000000);
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0X00000000);
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0X00000000);
      ImGui::PushStyleColor(ImGuiCol_DragDropTarget, 0X00000000);
    }

    int id = int(getUniqueID());

    bool active = false;

    ImGui::PushID(id);

    ImGui::PushStyleColor(ImGuiCol_Border, 0X00000000);
    if (ImGui::Button("", ImVec2(radius, radius) * 2)) {
      active = true;
    }
    ImGui::PopStyleColor();

    // Drag and Drop Target
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_pipe_output"))
      {
        active = true;
      }
      ImGui::EndDragDropTarget();
    }

    // Drag and Drop Source
    if (ImGui::BeginDragDropSource()) {
      if (!m_link) {
        ImGui::SetDragDropPayload("_pipe_input", nullptr, 0, ImGuiCond_Once);
      }
      else {
        ImGui::SetDragDropPayload("_pipe_output", nullptr, 0, ImGuiCond_Once);
      }
      if (!NodeEditor::Instance()->getSelectedOutput()) {
        active = true;
      }
      ImGui::EndDragDropSource();
    }

    ImGui::PopID();

    if (active && !m_isDataOnly) {
      updt = true;
      if (!m_link || NodeEditor::Instance()->getSelectedOutput()) {
        // start a new link
        NodeEditor::Instance()->setSelectedInput(owner()->input(name()));
      }
      else {
        // move the actual link
        NodeEditor::Instance()->setSelectedOutput(m_link->owner()->output(m_link->name()));
        Processor::disconnect(owner()->input(name()));
      }
    }

    ImGui::PopStyleColor(5);
 

  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.socket_border_width * w_scale);

  if (w_scale > 0.7F) {
    updt |= drawTweak();
    //ImGui::SameLine();
    //ImGui::Text(name());
  }

  ImGui::PopStyleVar();
  return updt;
}

//-------------------------------------------------------

std::string ProcessorInput::getLuaValue() {
  return "0";
}

//-------------------------------------------------------

bool chill::UndefInput::drawTweak() {
  ImGui::SameLine();

  ImGui::Text("%s", name());

  return false;
}

//-------------------------------------------------------

bool chill::BoolInput::drawTweak() {
  ImGui::SameLine();

  bool before = m_value;
  bool value_changed = false;

  if (!m_link) {
    value_changed = ImGui::Checkbox(name(), &m_value);
  }
  else {
    ImGui::Text("%s", name());
  }

  return value_changed || m_value != before;
}

//-------------------------------------------------------

bool chill::IntInput::drawTweak() {
  ImGui::SameLine();
  std::string name_str = std::string(name());
  std::string label = "##" + name_str;
  std::string format = name_str + ": %" + std::to_string(24 - name_str.size()) + ".0f";

  int before = m_value;
  bool value_changed = false;

  if (!m_link) {
    if (m_alt) {
      value_changed = ImGui::SliderInt(label.c_str(), &m_value, m_min, m_max, format.c_str());
    }
    else {
      value_changed = ImGui::DragInt(label.c_str(), &m_value, float(m_step), m_min, m_max, format.c_str());
    }
  }
  else {
    ImGui::Text("%s", name());
  }

  if (value_changed) {
    m_value = std::min(m_max, std::max(m_min, m_value));
  }

  return m_value != before;
}

//-------------------------------------------------------

bool chill::ListInput::drawTweak() {
  ImGui::SameLine();
  std::string name_str = std::string(name());
  std::string label = "##" + name_str;
  std::string format = name_str + ": %" + std::to_string(24 - name_str.size()) + ".0f";

  int before = m_value;
  bool value_changed = false;


  ImVec2 cursor = ImGui::GetCursorPos();
  ImGui::Text(" %s:", name_str.c_str());

  ImGui::SetCursorPos(cursor + ImVec2(0, 1.5F * ImGui::CalcTextSize(name()).y));

  if (!m_link) {

    ImGui::Combo(label.c_str(), &m_value,
                 [](void* data, int idx, const char** out_text) {
      *out_text = static_cast<const std::vector<std::string>*>(data)->at(idx).c_str();
      return true;
    }, reinterpret_cast<void*> (&m_values),
    static_cast<int>(m_values.size()));
  } else {
    ImGui::Text("%s", name());
  }

  if (value_changed) {
    m_value = std::min(m_max, std::max(m_min, m_value));
  }

  return m_value != before;
}

//-------------------------------------------------------

bool chill::PathInput::drawTweak() {
  ImGui::SameLine();
  std::string name_str = std::string(name());
  std::string label = "##" + name_str;
  std::string format = name_str + ": %" + std::to_string(24 - name_str.size()) + ".3g";

  const size_t path_max_size = 512;
  char string[path_max_size];
  strncpy(string, m_value.c_str(), path_max_size);

  std::string before = m_value;
  bool value_changed = false;

  ImVec2 cursor = ImGui::GetCursorPos();

  ImGui::Text(" %s:", name_str.c_str());
  cursor += ImVec2(0, 1.5F * ImGui::CalcTextSize(name()).y);

  float item_width = ImGui::CalcItemWidth();
  ImGui::PushItemWidth(item_width * 3.F / 4.F);

  if (!m_link) {
    ImGui::SetCursorPos(cursor + ImVec2(item_width * 1.F / 4.F, 0));
    ImGui::Text(fs::path(string).filename().generic_string().c_str());

    ImGui::SetCursorPos(cursor);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(73/255.F, 193/255.F, 194/255.F, 1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.processor_title_color);
    ImGui::PushItemWidth(item_width * 1.F / 4.F);

    if (ImGui::Button(" ... ##")) {
      std::string fullpath = openFileDialog(OFD_FILTER_ALL);
      if (!fullpath.empty()) {
        std::replace(fullpath.begin(), fullpath.end(), '\\', '/');
        m_value = fullpath.c_str();
        value_changed = true;
      }
    }
    ImGui::PopStyleColor(2);
  } else {
    ImGui::Text("%s", name());
  }
  return value_changed || m_value != before;
}

//-------------------------------------------------------

bool chill::ScalarInput::drawTweak() {
  ImGui::SameLine();

  std::string name_str = std::string(name());
  std::string label = "##" + name_str;
  std::string format = name_str + ": %" + std::to_string(24 - name_str.size()) + ".4g";

  float before = m_value;
  bool value_changed = false;
  
  if (!m_link) {
    if (m_alt) {
      value_changed = ImGui::SliderFloat(label.c_str(), &m_value, m_min, m_max, format.c_str());
    }
    else {
      value_changed = ImGui::DragFloat(label.c_str(), &m_value, m_step, m_min, m_max, format.c_str());
    }
  }
  else {
    ImGui::Text("%s", name());
  }

  if (value_changed) {
    m_value = std::min(m_max, std::max(m_min, m_value));
  }

  return value_changed || m_value != before;
}

//-------------------------------------------------------

bool chill::ShapeInput::drawTweak() {
  ImGui::SameLine();
  ImGui::Text("%s", name());
  return false;
}

//-------------------------------------------------------

bool chill::StringInput::drawTweak() {
  ImGui::SameLine();

  std::string name_str = std::string(name());
  std::string label = "##" + name_str;
  std::string format = name_str + ": %" + std::to_string(24 - name_str.size()) + ".3f";

  const size_t path_max_size = 512;
  char string[path_max_size];
  strncpy(string, m_value.c_str(), path_max_size);

  std::string before = m_value;
  bool value_changed = false;

  if (!m_link) {
    if (m_alt) {
      value_changed = ImGui::InputTextWithHint(("##" + std::to_string(getUniqueID())).c_str(), name(),string, 512);
    } else {
      value_changed = ImGui::InputTextWithHint(("##" + std::to_string(getUniqueID())).c_str(), name(), string, 512);
    }
    if (value_changed) {
      m_value = string;
    }
  } else {
    ImGui::Text("%s", name());
  }
  return value_changed || m_value != before;
}

//-------------------------------------------------------

bool chill::Vec4Input::drawTweak() {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  float w_scale = window->FontWindowScale;

  ImGui::SameLine();
  ImGui::Text("%s", name());
  ImVec2 current = ImGui::GetCursorPos();

  float before[4];
  std::memcpy(before, m_value, sizeof(int)*4);

  bool value_changed = false;

  if (!m_link) {
    float padding = (style.socket_radius + style.socket_border_width + style.ItemSpacing.x) * w_scale;
    ImGui::SetCursorPosX(current.x + padding);

    float item_width = ImGui::CalcItemWidth();
    std::string label = "##" + std::string(name());

    if (m_alt) {
      ImGuiColorEditFlags flags = ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaBar;
      ImGui::PushItemWidth(item_width);
      value_changed = ImGui::ColorPicker4(label.c_str(), m_value, flags);
      ImGui::PopItemWidth();
    }
    else {
      const char* format = "%0.3f";
      value_changed = ImGui::DragFloat4(label.c_str(), m_value, m_step, m_min, m_max, format);
    }
  }
  if (value_changed) {
    for (int i = 0; i < 4; ++i) {
      m_value[i] = std::min(m_max, std::max(m_min, m_value[i]));
    }
  }

  for (int i = 0; i < 4; i++) {
    value_changed |= before[i] != m_value[i];
  }

  return value_changed;
}

//-------------------------------------------------------

bool Vec3Input::drawTweak() {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  float w_scale = window->FontWindowScale;

  ImGui::SameLine();
  ImGui::Text("%s", name());
  ImVec2 current = ImGui::GetCursorPos();

  float before[3];
  std::memcpy(before, m_value, sizeof(int) * 3);
  bool value_changed = false;

  if (!m_link) {
    float padding = (style.socket_radius + style.socket_border_width + style.ItemSpacing.x) * w_scale;
    ImGui::SetCursorPosX(current.x + padding);

    std::string label = "##" + std::string(name());
    const char* format = "%0.3f";
    value_changed = ImGui::DragFloat3(label.c_str(), m_value, m_step, m_min, m_max, format);
  }

  if (value_changed) {
    for (int i = 0; i < 3; ++i) {
      m_value[i] = std::min(m_max, std::max(m_min, m_value[i]));
    }
  }

  for (int i = 0; i < 3; i++) {
    value_changed |= before[i] != m_value[i];
  }

  return value_changed;
}

} // namespace chill
