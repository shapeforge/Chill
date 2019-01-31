#include "Processor.h"
#include "ProcessingGraph.h"
#include "IOs.h"

namespace Chill {

  Processor::Processor(Processor &copy) {
    m_name = copy.m_name;
    m_color = copy.m_color;
    m_owner = copy.m_owner;

    for (AutoPtr<ProcessorInput> input : copy.m_inputs) {
      addInput(input->clone());
    }

    for (AutoPtr<ProcessorOutput> output : copy.m_outputs) {
      addOutput(output->clone());
    }
  }

  Processor::~Processor() {
    std::cout << "~" << name() << std::endl;
    for (AutoPtr<ProcessorInput> input : m_inputs) {
      m_owner->disconnect(input);
    }
    for (AutoPtr<ProcessorOutput> output : m_outputs) {
      m_owner->disconnect(output);
    }
    m_owner = NULL;
    m_inputs.clear();
    m_outputs.clear();
  }

  AutoPtr<ProcessorInput> Processor::input(std::string name_) {
    for (AutoPtr<ProcessorInput> input : m_inputs) {
      if (input->name() == name_) {
        return input;
      }
    }
    return AutoPtr<ProcessorInput>(NULL);
  }

  AutoPtr<ProcessorOutput> Processor::output(std::string name_) {
    for (AutoPtr<ProcessorOutput> output : m_outputs) {
      if (output->name() == name_) {
        return output;
      }
    }
    return AutoPtr<ProcessorOutput>(NULL);
  }


  AutoPtr<ProcessorInput> Processor::addInput(AutoPtr<ProcessorInput> input_) {
    input_->setOwner(this);

    // rename the input if the name already exists (should only appears in GroupProcessors)
    std::string base = input_->name();
    std::string name = base;
    int nb = 1;
    while (!input(name).isNull()) {
      name = base + "_" + std::to_string(nb++);
    }
    input_->setName(name);

    m_inputs.push_back(input_);
    return input_;
  }

  template <typename ... Args>
  AutoPtr<ProcessorInput> Processor::addInput(std::string name_, IOType::IOType type_, Args&& ... args_) {
    return addInput(ProcessorInput::create(name_, type_, args_...));
  }

  AutoPtr<ProcessorOutput> Processor::addOutput(AutoPtr<ProcessorOutput> output_) {
    output_->setOwner(this);

    // rename the output if the name already exists (should only appears in GroupProcessors)
    std::string base = output_->name();
    std::string name = base;
    int nb = 1;
    while (!output(name).isNull()) {
      name = base + "_" + std::to_string(nb++);
    }
    output_->setName(name);

    m_outputs.push_back(output_);
    return output_;
  }

  AutoPtr<ProcessorOutput> Processor::addOutput(std::string name_, IOType::IOType type_) {
    return addOutput(ProcessorOutput::create(name_, type_));
  }

  void Processor::removeInput(AutoPtr<ProcessorInput>& input) {
    m_owner->disconnect(input);
    m_inputs.erase(remove(m_inputs.begin(), m_inputs.end(), input), m_inputs.end());
  }

  void Processor::removeOutput(AutoPtr<ProcessorOutput>& output) {
    m_owner->disconnect(output);
    m_outputs.erase(remove(m_outputs.begin(), m_outputs.end(), output), m_outputs.end());
  }

  void Processor::save(std::ofstream& stream) {
    ImVec4 color4vec = ImGui::ColorConvertU32ToFloat4(color());
    stream << "p_" << getUniqueID() << " = Processor({" <<
              "name = '" << m_name << "'" <<
              ", x = " << getPosition().x <<
              ", y = " << getPosition().y <<
              ", color = {" << int(color4vec.x * 255) << ", " << int(color4vec.y * 255) << ", " << int(color4vec.z * 255) << "}"
              "})" << std::endl;
    
    /* Saving I/Os is not usefull in general case*/
    // Save inputs
    for (AutoPtr<ProcessorInput> input : m_inputs) {
      input->save(stream);
      stream << "p_" << getUniqueID() << ":add(i_" << input->getUniqueID() << ")" << std::endl;
    }
    // Save outputs
    for (AutoPtr<ProcessorOutput> output : m_outputs) {
      output->save(stream);
      stream << "p_" << getUniqueID() << ":add(o_" << output->getUniqueID() << ")" << std::endl;
    }
  }
  
  void Processor::iceSL(std::ofstream& stream) {
    
  }


  bool Processor::isEmiter() {
    return m_emit;
  }
};

bool Chill::Processor::draw() {
  ImGui::PushID(int(getUniqueID()));

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImGuiIO io = ImGui::GetIO();

  ImVec2 size = m_size * m_scale;

  ImVec2 min_pos = ImGui::GetCursorScreenPos();
  ImVec2 max_pos = min_pos + size;

  float border_width = style.processor_border_width * m_scale;
  float rounding_corners = style.processor_rounding_corners * m_scale;

  // shadow
  if (m_selected || m_scale > 0.5f) {
    draw_list->AddRectFilled(
      min_pos + ImVec2(-5, -5) * m_scale,
      max_pos + ImVec2(5, 5) * m_scale,
      m_selected ? style.processor_shadow_selected_color : style.processor_shadow_color,
      rounding_corners + 5.0f, style.processor_rounded_corners);
  }

  // border
  ImVec2 border = ImVec2(style.processor_border_width, style.processor_border_width) * m_scale / 2.0f;
  draw_list->AddRect(
    min_pos - border,
    max_pos + border,
    m_selected ? style.processor_selected_color : m_color,
    rounding_corners, style.processor_rounded_corners,
    border_width
  );

  // background
  draw_list->AddRectFilled(min_pos, max_pos, style.processor_bg_color, rounding_corners, style.processor_rounded_corners);

  float height = ImGui::GetCursorPosY();

  float padding = (style.socket_radius + style.socket_border_width + style.ItemSpacing.x) * m_scale;

  ImGui::PushItemWidth(m_size.x * m_scale - padding * 2);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * m_scale);

  ImGui::SetWindowFontScale(m_scale);


  ImGui::BeginGroup();
  // draw title
  ImVec2 title_size(style.processor_width, style.processor_title_height);
  title_size *= m_scale;

  draw_list->AddRectFilled(min_pos, min_pos + title_size,
    m_color,
    rounding_corners, style.processor_rounded_corners & 3);

  if (!m_edit) {
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0, 0, 0, 255));
    if (ImGui::ButtonEx((name() + "##" + std::to_string(getUniqueID())).c_str(), title_size, ImGuiButtonFlags_PressedOnDoubleClick ))
      m_edit = true;
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
  }
  else {
    strcpy(m_title, name().c_str());
    if (ImGui::InputText(("##" + std::to_string(getUniqueID())).c_str(), m_title, 32)) {
      setName(m_title);
    }
    else if (!m_selected) {
      m_edit = false;
    }
  }

  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20);

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padding);
  
  // Drag and Drop Target
  bool value_changed = false;
  if (ImGui::BeginDragDropTarget())
  {
    float col[4];
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
    {
      memcpy((float*)col, payload->Data, sizeof(float) * 3);
      value_changed = true;
    }
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
    {
      memcpy((float*)col, payload->Data, sizeof(float) * 4);
      value_changed = true;
    }
    if (value_changed) m_color = ImColor(col[0], col[1], col[2]);
    ImGui::EndDragDropTarget();
  }
  //ImGui::PopID();

  //ImGui::PushClipRect(min_pos - border - ImVec2(0.5, 0.5), max_pos + border + ImVec2(0.5, 0.5), true);

  float y = ImGui::GetCursorPosY();
  // draw inputs
  m_dirty = false;
  ImGui::BeginGroup();
  for (AutoPtr<ProcessorInput> input : m_inputs) {
    m_dirty |= input->draw();
  }
  ImGui::EndGroup();

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size.x);
  ImGui::BeginGroup();
  for (AutoPtr<ProcessorOutput> output : m_outputs) {
    output->draw();
  }
  ImGui::EndGroup();

  //ImGui::PopClipRect();
  
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();

  ImGui::EndGroup();
  ImGui::PopItemWidth();
  ImGui::PopStyleVar();

  height = ImGui::GetCursorPosY() - height + padding;

  m_size.x = style.processor_width;
  m_size.y = height / m_scale;

  ImGui::PopID();
  ImGui::SetWindowFontScale(1.0f);

  return m_edit;
}




Chill::Multiplexer::Multiplexer() {
  setName("Multiplexer");
  m_size = ImVec2(1, 1);
  m_size *= style.processor_title_height;

  addInput("i", IOType::UNDEF);
  addOutput("o", IOType::UNDEF);
}

bool Chill::Multiplexer::draw() {

  ImGui::PushID(int(getUniqueID()));

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImGuiIO io = ImGui::GetIO();

  ImVec2 size = m_size * m_scale;
  
  ImVec2 min_pos = ImGui::GetCursorScreenPos();
  ImVec2 max_pos = min_pos + size;

  float border_width = style.processor_border_width * m_scale;
  float rounding_corners = style.processor_rounding_corners * m_scale;


  // shadow
  if (m_selected || m_scale > 0.5f) {
    draw_list->AddRectFilled(
      min_pos + ImVec2(-5, -5) * m_scale,
      max_pos + ImVec2(5, 5) * m_scale,
      m_selected ? style.processor_shadow_selected_color : style.processor_shadow_color,
      rounding_corners * 2.0f, style.processor_rounded_corners);
  }

  // border & background
  ImVec2 border = ImVec2(style.processor_border_width, style.processor_border_width) * m_scale / 2.0f;
  draw_list->AddRectFilled(
    min_pos - border,
    max_pos + border,
    m_selected ? style.processor_selected_color : color(),
    rounding_corners, style.processor_rounded_corners
  );

  // background
  // draw_list->AddRectFilled(min_pos, max_pos, style.processor_bg_color, rounding_corners, style.processor_rounded_corners);

  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20);
  float x = ImGui::GetCursorPosX();
  float y = ImGui::GetCursorPosY() + size.y/2 - m_scale * style.socket_radius;
  
  // draw inputs
  ImGui::SetCursorPosX(x);
  ImGui::SetCursorPosY(y);
  
  ImGui::BeginGroup();
  for (AutoPtr<ProcessorInput> input : inputs()) {
    input->draw();
    if (!input->m_link.isNull()) {
      setColor(input->m_link->owner()->color());
    }
  }
  ImGui::EndGroup();

  ImGui::SetCursorPosY(y);
  ImGui::SetCursorPosX(x + size.x);
  // draw ouputs
  ImGui::BeginGroup();
  for (AutoPtr<ProcessorOutput> output : outputs()) {
    output->draw();
  }
  ImGui::EndGroup();



  ImGui::PopID();
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();

  return m_edit;
}

void Chill::Multiplexer::iceSL(std::ofstream& _stream) {
  //write the current Id of the node

  std::string lua = "--[[ " + name() + " ]]--\n";
  lua += "setfenv(1, _G0)  --go back to global initialization\n";
  lua += "__currentNodeId = ";
  lua += std::to_string((int64_t)this);
  lua += "\n";

  for (auto input : inputs()) {
    // tweak
    if (input->m_link.isNull()) {
      lua += "__input[\"" + std::string(input->name()) + "\"] = nil\n";
    }
    // input
    else {
      std::string s2 = std::to_string((int64_t)input->m_link->owner());
      lua += "__input[\"" + std::string(input->name()) + "\"] = " + input->m_link->name() + s2 + "\n";
    }
  }

  //TODO: CLEAN THIS !!!!
  lua += std::string("\
_Gcurrent = {} -- clear _Gcurrent\n\
setmetatable(_Gcurrent, { __index = _G0 }) --copy index from _G0\n\
setfenv(1, _Gcurrent)    --set it\n\
");

  lua += "if (isDirty({__currentNodeId";

  for (auto input : inputs()) {
    if (!input->m_link.isNull()) {
      std::string s2 = std::to_string((int64_t)input->m_link->owner());
      lua += ", " + s2;
    }
  }

  lua += "})) then\n\
setDirty(__currentNodeId)\n";
  lua += "output('o','UNDEF', input('i', 'UNDEF'))";
  lua += "\nend";

  _stream << lua << std::endl;
}