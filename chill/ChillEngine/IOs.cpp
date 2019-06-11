#include "IOs.h"
#include "NodeEditor.h"

#include "FileDialog.h"

namespace Chill {
  ProcessorOutput::~ProcessorOutput() {
    for (AutoPtr<ProcessorInput> input : m_links)
      NodeEditor::Instance()->getCurrentGraph()->disconnect(input);
    m_links.clear();

  };
  
  bool ProcessorOutput::draw() {
    float radius = (style.socket_radius + style.socket_border_width) * owner()->m_scale;
    float full_radius = (style.socket_radius + style.socket_border_width) * owner()->m_scale;

    ImVec2 pos = ImGui::GetCursorPos();
    ImVec2 text_size = ImGui::CalcTextSize(name());

    ImGui::SetCursorPosX(pos.x - text_size.x - style.ItemSpacing.x * m_scale - full_radius);
    ImGui::SetCursorPosY(pos.y - text_size.y/2 + full_radius);

    ImGui::Text(name());

    ImGui::SetCursorPos(ImVec2(pos.x - full_radius, pos.y));
    
    setPosition(ImVec2(pos.x, pos.y + full_radius));

    ImGui::PushStyleColor(ImGuiCol_Text, color());
    ImGui::PushStyleColor(ImGuiCol_Button, color());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color() & 0xAAFFFFFF);

    ImGui::PushStyleColor(ImGuiCol_DragDropTarget, style.pipe_selected_color);

    int id = int(getUniqueID());
    ImGui::PushID(id);

    ImGui::PushStyleColor(ImGuiCol_Border, 0x00000000);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.socket_border_width * owner()->m_scale);
    if (ImGui::Button("", ImVec2(radius, radius) * 2)) {
      NodeEditor::Instance()->setSelectedOutput(owner()->output(name()));
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    if (ImGui::BeginDragDropSource()) {
      NodeEditor::Instance()->setSelectedOutput(owner()->output(name()));
      ImGui::SetDragDropPayload("_pipe_output", NULL, 0, ImGuiCond_Once);
      ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
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

  AutoPtr<ProcessorOutput> ProcessorOutput::create(std::string name_, IOType::IOType type_ = IOType::UNDEF) {
    AutoPtr<ProcessorOutput> output;
    switch (type_) {
    case IOType::BOOLEAN:
      output = AutoPtr<ProcessorOutput>(new BoolOutput());
      break;
    case IOType::INTEGER:
      output = AutoPtr<ProcessorOutput>(new IntOutput());
      break;
    case IOType::PATH:
      output = AutoPtr<ProcessorOutput>(new PathOutput());
      break;
    case IOType::SCALAR:
      output = AutoPtr<ProcessorOutput>(new ScalarOutput());
      break;
    case IOType::SHAPE:
      output = AutoPtr<ProcessorOutput>(new ShapeOutput());
      break;
    case IOType::STRING:
      output = AutoPtr<ProcessorOutput>(new StringOutput());
      break;
    case IOType::VEC3:
      output = AutoPtr<ProcessorOutput>(new Vec3Output());
      break;
    case IOType::VEC4:
      output = AutoPtr<ProcessorOutput>(new Vec4Output());
      break;
    case IOType::UNDEF:
    default:
      output = AutoPtr<ProcessorOutput>(new UndefOutput());
      break;
    }
    output->setName(name_);
    output->setType(type_);
    return output;
  }
}


namespace Chill {
  ProcessorInput::~ProcessorInput() {
    NodeEditor::Instance()->getCurrentGraph()->disconnect(m_link);
  };

  bool ProcessorInput::draw() {
    bool updt = false;

    float radius = (style.socket_radius + style.socket_border_width) * owner()->m_scale;
    float full_radius = (style.socket_radius + style.socket_border_width) * owner()->m_scale;

    ImVec2 pos = ImGui::GetCursorPos();
    ImVec2 text_size = ImGui::CalcTextSize(name());

    ImGui::SetCursorPos(ImVec2(pos.x - full_radius, pos.y));
    setPosition(ImVec2(pos.x, pos.y + full_radius));

    ImGui::PushStyleColor(ImGuiCol_Text,          color());
    ImGui::PushStyleColor(ImGuiCol_Button,        color());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  color() & 0xAAFFFFFF);

    ImGui::PushStyleColor(ImGuiCol_DragDropTarget, style.pipe_selected_color);

    int id = int(getUniqueID());

    bool active = false;
    
    ImGui::PushID(id);
    ImGui::PushStyleColor(ImGuiCol_Border, 0X00000000);
    if (ImGui::Button("", ImVec2(radius, radius) * 2)) {
      active = true;
    }
    ImGui::PopStyleColor();


    // Drag and Drop Target
    if (ImGui::BeginDragDropTarget())
    {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_pipe_output")) {
        active = true;
      }
      ImGui::EndDragDropTarget();
    }

    // Drag and Drop Source
      if (ImGui::BeginDragDropSource()) {
        if (m_link.isNull()) {
          ImGui::SetDragDropPayload("_pipe_input", NULL, 0, ImGuiCond_Once);
        }
        else {
          ImGui::SetDragDropPayload("_pipe_output", NULL, 0, ImGuiCond_Once);
        }
        if (NodeEditor::Instance()->getSelectedOutput().isNull()) {
          active = true;
        }
        ImGui::EndDragDropSource();
    }

    ImGui::PopID();

    if (active) {
      updt = true;
      if (m_link.isNull()) {
        // start a new link
        NodeEditor::Instance()->setSelectedInput(owner()->input(name()));
      }
      else {
        // move the actual link
        NodeEditor::Instance()->setSelectedOutput(m_link->owner()->output(m_link->name()));
        NodeEditor::Instance()->getCurrentGraph()->disconnect(owner()->input(name()));
      }
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.socket_border_width * owner()->m_scale);
    updt |= drawTweak();
    ImGui::PopStyleVar();
    return updt;
  }

  std::string ProcessorInput::getLuaValue() {
    return "0";
  }
}






bool Chill::UndefInput::drawTweak()
{
  ImGui::SameLine();

  ImGui::Text(name());

  return false;
}

bool Chill::BoolInput::drawTweak()
{
  ImGui::SameLine();

  bool before = m_value;
  bool value_changed = false;

  if (m_link.isNull()) {
    value_changed = ImGui::Checkbox(name(), &m_value);
  }
  else {
    ImGui::Text(name());
  }

  return value_changed || m_value != before;
}

bool Chill::IntInput::drawTweak()
{
  ImGui::SameLine();
  std::string name_str = std::string(name());
  std::string label = "##" + name_str;
  std::string format = name_str + ": %" + std::to_string(24 - name_str.size()) + ".0f";

  int before = m_value;
  bool value_changed = false;

  if (m_link.isNull()) {
    if (m_alt) {
      value_changed = ImGui::SliderInt(label.c_str(), &m_value, m_min, m_max, format.c_str());
    }
    else {
      value_changed = ImGui::DragInt(label.c_str(), &m_value, float(m_step), m_min, m_max, format.c_str());
    }
  }
  else {
    ImGui::Text(name());
  }

  if (value_changed) {
    m_value = std::min(m_max, std::max(m_min, m_value));
  }

  if (m_value != before)
  std::cout << m_value << "vs" << before << std::endl;

  return m_value != before;
}

bool Chill::ListInput::drawTweak()
{
  ImGui::SameLine();
  std::string name_str = std::string(name());
  std::string label = "##" + name_str;
  std::string format = name_str + ": %" + std::to_string(24 - name_str.size()) + ".0f";

  int before = m_value;
  bool value_changed = false;

  if (m_link.isNull()) {


    ImGui::Combo(name(), &m_value,
      [](void* data, int idx, const char** out_text) {
        *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str();
        return true;
      }, reinterpret_cast<void*> (&m_values),
      m_values.size());
  } else {
    ImGui::Text(name());
  }

  if (value_changed) {
    m_value = std::min(m_max, std::max(m_min, m_value));
  }

  if (m_value != before)
    std::cout << m_value << "vs" << before << std::endl;

  return m_value != before;
}


bool Chill::PathInput::drawTweak()
{
  ImGui::SameLine();
  ImGui::Text(name());

  std::string name_str = std::string(name());
  std::string label = "##" + name_str;
  std::string format = name_str + ": %" + std::to_string(24 - name_str.size()) + ".3f";

  char string[512];
  strcpy(string, m_value.c_str());

  std::string before = m_value;
  bool value_changed = false;

  if (m_link.isNull()) {
    if (m_alt) {
      value_changed = ImGui::InputText(("##" + std::to_string(getUniqueID())).c_str(), string, 512);
    } else {
      value_changed = ImGui::InputText(("##" + std::to_string(getUniqueID())).c_str(), string, 512);
    }
    if (value_changed) {
      m_value = string;
    }
    if (ImGui::Button("...##")) {
      std::string fullpath = openFileDialog(OFD_FILTER_GRAPHS);
      if (!fullpath.empty()) {
        std::replace(fullpath.begin(), fullpath.end(), '\\', '/');
        m_value = fullpath.c_str();
      }
    }
  } else {
    ImGui::Text(name());
  }
  return value_changed || m_value != before;
}


bool Chill::ScalarInput::drawTweak()
{
  ImGui::SameLine();
  std::string name_str = std::string(name());
  std::string label = "##" + name_str;
  std::string format = name_str + ": %" + std::to_string(24 - name_str.size()) + ".3f";
 
  float before = m_value;
  bool value_changed = false;
  
  if (m_link.isNull()) {
    if (m_alt) {
      value_changed = ImGui::SliderFloat(label.c_str(), &m_value, m_min, m_max, format.c_str());
    }
    else {
      value_changed = ImGui::DragFloat(label.c_str(), &m_value, m_step, m_min, m_max, format.c_str());
    }
  }
  else {
    ImGui::Text(name());
  }

  if (value_changed) {
    m_value = std::min(m_max, std::max(m_min, m_value));
  }

  return value_changed || m_value != before;
}


bool Chill::ShapeInput::drawTweak()
{
  ImGui::SameLine();
  ImGui::Text(name());
  return false;
}


bool Chill::StringInput::drawTweak()
{
  ImGui::SameLine();
  ImGui::Text(name());

  std::string name_str = std::string(name());
  std::string label = "##" + name_str;
  std::string format = name_str + ": %" + std::to_string(24 - name_str.size()) + ".3f";

  char string[512];
  strcpy(string, m_value.c_str());

  std::string before = m_value;
  bool value_changed = false;

  if (m_link.isNull()) {
    if (m_alt) {
      value_changed = ImGui::InputText(("##" + std::to_string(getUniqueID())).c_str(), string, 512);
    } else {
      value_changed = ImGui::InputText(("##" + std::to_string(getUniqueID())).c_str(), string, 512);
    }
    if (value_changed) {
      m_value = string;
    }
  } else {
    ImGui::Text(name());
  }
  return value_changed || m_value != before;
}


bool Chill::Vec4Input::drawTweak()
{
  ImGui::SameLine();
  ImGui::Text(name());
  ImVec2 current = ImGui::GetCursorPos();

  float before[4];
  std::memcpy(before, m_value, sizeof(int)*4);

  bool value_changed = false;

  if (m_link.isNull()) {
    float padding = (style.socket_radius + style.socket_border_width + style.ItemSpacing.x) * owner()->m_scale;
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

bool Chill::Vec3Input::drawTweak()
{
  ImGui::SameLine();
  ImGui::Text(name());
  ImVec2 current = ImGui::GetCursorPos();

  float before[3];
  std::memcpy(before, m_value, sizeof(int) * 3);
  bool value_changed = false;

  if (m_link.isNull()) {
    float padding = (style.socket_radius + style.socket_border_width + style.ItemSpacing.x) * owner()->m_scale;
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