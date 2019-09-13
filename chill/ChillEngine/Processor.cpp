#include "Processor.h"
#include "ProcessingGraph.h"
#include "IOs.h"

namespace chill {

std::shared_ptr<ProcessorInput> Processor::addInput(std::shared_ptr<ProcessorInput> _input) {
  _input->setOwner(this);

  // rename the input if the name already exists (should only appears in GroupProcessors)
  std::string base = _input->name();
  std::string name = base;
  int nb = 1;
  while (input(name)) {
    name = base + "_" + std::to_string(nb++);
  }
  _input->setName(name);

  m_inputs.push_back(_input);
  return _input;
}

  Processor::Processor(Processor &copy) {
    m_name  = copy.m_name;
    m_color = copy.m_color;
    m_owner = copy.m_owner;

    for (std::shared_ptr<ProcessorInput> input : copy.m_inputs) {
      addInput(input->clone());
    }

    for (std::shared_ptr<ProcessorOutput> output : copy.m_outputs) {
      addOutput(output->clone());
    }
  }

  Processor::~Processor() {
    std::cout << "~" << name() << std::endl;
    for (std::shared_ptr<ProcessorInput> input : m_inputs) {
      m_owner->disconnect(input);
    }
    for (std::shared_ptr<ProcessorOutput> output : m_outputs) {
      m_owner->disconnect(output);
    }
    m_owner = nullptr;
    m_inputs.clear();
    m_outputs.clear();
  }

  std::shared_ptr<ProcessorInput> Processor::input(std::string name_) {
    for (std::shared_ptr<ProcessorInput> input : m_inputs) {
      if (input->name() == name_) {
        return input;
      }
    }
    return std::shared_ptr<ProcessorInput>(nullptr);
  }

  std::shared_ptr<ProcessorOutput> Processor::output(std::string name_) {
    for (std::shared_ptr<ProcessorOutput> output : m_outputs) {
      if (output->name() == name_) {
        return output;
      }
    }
    return std::shared_ptr<ProcessorOutput>(nullptr);
  }

  std::shared_ptr<ProcessorOutput> Processor::addOutput(std::shared_ptr<ProcessorOutput> output_) {
    output_->setOwner(this);

    // rename the output if the name already exists (should only appears in GroupProcessors)
    std::string base = output_->name();
    std::string name = base;
    int nb = 1;
    while (output(name)) { // ! isNull
      name = base + "_" + std::to_string(nb++);
    }
    output_->setName(name);

    m_outputs.push_back(output_);
    return output_;
  }

  std::shared_ptr<ProcessorOutput> Processor::addOutput(std::string _name, IOType::IOType _type, bool _emitable) {
    return addOutput(ProcessorOutput::create(_name, _type, _emitable));
  }

  void Processor::removeInput(std::shared_ptr<ProcessorInput>& input) {
    m_owner->disconnect(input);
    m_inputs.erase(remove(m_inputs.begin(), m_inputs.end(), input), m_inputs.end());
  }

  void Processor::removeOutput(std::shared_ptr<ProcessorOutput>& output) {
    m_owner->disconnect(output);
    m_outputs.erase(remove(m_outputs.begin(), m_outputs.end(), output), m_outputs.end());
  }


  bool Processor::connect(std::shared_ptr<Processor> from, const std::string output_name,
    std::shared_ptr<Processor> to, const std::string input_name)
  {
    // check if the processors exists
    if (!from || !to) {
      return false;
    }

    std::shared_ptr<ProcessorOutput> output = from->output(output_name);
    std::shared_ptr<ProcessorInput>  input  = to->input(input_name);

    return connect(output, input);
  }

  bool Processor::connect(std::shared_ptr<ProcessorOutput> from, std::shared_ptr<ProcessorInput> to)
  {
    // check if the processors i/o exists
    if (!from || !to) {
      return false;
    }

    // check if the processors comes from the same graph
    if (from->owner()->owner() != to->owner()->owner()) {
      return false;
    }

    // check if input already connected
    if (to->m_link) {
      disconnect(to);
    }

    // if the new pipe create a cycle
    if (areConnected(to->owner(), from->owner())) {
      return false;
    }

    to->m_link = from;
    from->m_links.push_back(to);

    return true;
  }

  void Processor::disconnect(std::shared_ptr<ProcessorInput> to) {
    if (!to) return;

    std::shared_ptr<ProcessorOutput> from = to->m_link;
    if (from) {
      from->m_links.erase(std::remove(from->m_links.begin(), from->m_links.end(), to), from->m_links.end());
    }

    to->m_link = std::shared_ptr<ProcessorOutput>(nullptr);
  }

  void Processor::disconnect(std::shared_ptr<ProcessorOutput> from) {
    if (!from) return;

    for (std::shared_ptr<ProcessorInput> to : from->m_links) {
      if (to) {
        to->m_link = std::shared_ptr<ProcessorOutput>(nullptr);
      }
    }
    from->m_links.clear();
  }

  bool Processor::areConnected(Processor * from, Processor * to)
  {
    // Raw pointers, because m_owner is a raw pointer
    std::queue<Processor*> toCheck;
    toCheck.push(to);

    while (!toCheck.empty()) {
      Processor* current = toCheck.front();
      toCheck.pop();

      if (current == from) {
        return true;
      }
      for (std::shared_ptr<ProcessorInput> input : current->inputs()) {
        if (input->m_link) {
          std::shared_ptr<ProcessorOutput> output = input->m_link;
          toCheck.push(output->owner());
        }
      }
    }
    return false;
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
    for (std::shared_ptr<ProcessorInput> input : m_inputs) {
      input->save(stream);
      stream << "p_" << getUniqueID() << ":add(i_" << input->getUniqueID() << ")" << std::endl;
    }
    // Save outputs
    for (std::shared_ptr<ProcessorOutput> output : m_outputs) {
      output->save(stream);
      stream << "p_" << getUniqueID() << ":add(o_" << output->getUniqueID() << ")" << std::endl;
    }
  }
  
  void Processor::iceSL(std::ofstream& ) {}
};

bool chill::Processor::draw() {
  m_dirty = false;

  ImGui::PushID(int(getUniqueID()));

  ImGuiWindow* window = ImGui::GetCurrentWindow();
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  float w_scale = window->FontWindowScale;

  ImVec2 size = m_size * w_scale;

  ImVec2 min_pos = ImGui::GetCursorScreenPos();
  ImVec2 max_pos = min_pos + size;

  float border_width = style.processor_border_width * w_scale;
  float rounding_corners = style.processor_rounding_corners * w_scale;

  // shadow
  if (m_selected || w_scale > 0.5F) {
    draw_list->AddRectFilled(
      min_pos + ImVec2(10, 10) * w_scale,
      max_pos + ImVec2(10, 10) * w_scale,
      style.processor_shadow_color,
      rounding_corners + 5.0F, style.processor_rounded_corners);
  }

  // border
  ImVec2 border = ImVec2(style.processor_border_width, style.processor_border_width) * w_scale / 2.0F;
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

  float padding = (style.socket_radius + style.socket_border_width + style.ItemSpacing.x) * w_scale;

  ImGui::PushItemWidth(m_size.x * w_scale - padding * 2.F);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * w_scale);

  ImGui::BeginGroup();

  float button_size = style.processor_title_height / 1.2F * w_scale;

  // draw title
  ImVec2 title_size(style.processor_width, style.processor_title_height);
  title_size *= w_scale;

  draw_list->AddRectFilled(min_pos, min_pos + title_size,
    m_color,
    rounding_corners, style.processor_rounded_corners & 3);

  if (w_scale > 0.7F) {
    if (!m_edit) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 255));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 255));


      if (ImGui::ButtonEx((name() + "##" + std::to_string(getUniqueID())).c_str(), title_size - ImVec2(2 * button_size, 0), ImGuiButtonFlags_PressedOnDoubleClick)) {
        m_edit = true;
      }
      ImGui::PopStyleColor();
      ImGui::PopStyleColor();
      ImGui::PopStyleColor();
      ImGui::PopStyleColor();
    } else {
      char title[32];
      strncpy(title, name().c_str(), 32);
      if (ImGui::InputText(("##" + std::to_string(getUniqueID())).c_str(), title, 32)) {
        setName(title);
      } else if (!m_selected) {
        m_edit = false;
      }
    }
    
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padding);
  } else {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + title_size[1] + padding);
  }
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20);

  
  
  // Drag and Drop Target
  bool value_changed = false;
  if (ImGui::BeginDragDropTarget())
  {
    float col[4];
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
    {
      memcpy(static_cast<float*>(col), payload->Data, sizeof(float) * 3);
      value_changed = true;
    }
    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
    {
      memcpy(static_cast<float*>(col), payload->Data, sizeof(float) * 4);
      value_changed = true;
    }
    if (value_changed) m_color = ImColor(col[0], col[1], col[2]);
    ImGui::EndDragDropTarget();
  }

  // DISABLE / EMIT

  bool isStateful = false;
  for (auto output : m_outputs) {
    if (output->type() == IOType::SHAPE) {
      isStateful = true;
      break;
    }
  }
  /*
  if (isStateful || isEmiter()) {
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (style.processor_title_height * w_scale - button_size)/2.0F);
    ImU32 color(m_state == DISABLED ? 0XFF0000CC : m_state == DEFAULT ? 0XFFCC7700 : m_state == EMITING ? 0XFF00CC00 : 0XFF888888);
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, button_size);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
    if (ImGui::Button("", ImVec2(button_size, button_size))) {
      setDirty(true);
      switch (m_state) {
      case DISABLED:
        m_state = DEFAULT;
        break;
      case DEFAULT:
        if (isEmiter())
          m_state = DISABLED;
        else
          m_state = EMITING;
        break;
      case EMITING:
        m_state = DISABLED;
        break;
      default:
        m_state = DEFAULT;
        break;
      }
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
  }
  */

  // draw inputs
  ImGui::BeginGroup();
  for (std::shared_ptr<ProcessorInput> input : m_inputs) {
    m_dirty |= input->draw();
  }
  ImGui::EndGroup();

  // draw outputs
  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size.x);
  ImGui::BeginGroup();
  for (std::shared_ptr<ProcessorOutput> output : m_outputs) {
    m_dirty |= output->draw();
  }
  ImGui::EndGroup();
  
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();

  ImGui::EndGroup();
  ImGui::PopItemWidth();
  ImGui::PopStyleVar();

  height = ImGui::GetCursorPosY() - height + padding;

  m_size.x = style.processor_width;
  m_size.y = height / w_scale;

  ImGui::PopID();

  return m_edit;
}




chill::Multiplexer::Multiplexer() {
  setName("Multiplexer");
  m_size = ImVec2(1, 1);
  m_size *= style.processor_title_height;

  addInput("i", IOType::UNDEF);
  addOutput("o", IOType::UNDEF);
}

bool chill::Multiplexer::draw() {

  ImGui::PushID(int(getUniqueID()));

  ImGuiWindow* window = ImGui::GetCurrentWindow();
  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  float w_scale = window->FontWindowScale;

  ImVec2 size = m_size * w_scale;
  
  ImVec2 min_pos = ImGui::GetCursorScreenPos();
  ImVec2 max_pos = min_pos + size;

  //float border_width = style.processor_border_width * w_scale;
  float rounding_corners = style.processor_rounding_corners * w_scale;


  // shadow
  if (m_selected || w_scale > 0.5F) {
    draw_list->AddRectFilled(
      min_pos + ImVec2(-5, -5) * w_scale,
      max_pos + ImVec2(5, 5) * w_scale,
      m_selected ? style.processor_shadow_selected_color : style.processor_shadow_color,
      rounding_corners * 2.0F, style.processor_rounded_corners);
  }

  // border & background
  ImVec2 border = ImVec2(style.processor_border_width, style.processor_border_width) * w_scale / 2.0F;
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
  float y = ImGui::GetCursorPosY() + size.y/2.0F - w_scale * style.socket_radius;
  
  // draw inputs
  ImGui::SetCursorPosX(x);
  ImGui::SetCursorPosY(y);
  
  ImGui::BeginGroup();
  for (std::shared_ptr<ProcessorInput> input : inputs()) {
    input->draw();
    if (input->m_link) {
      setColor(input->m_link->owner()->color());
    }
  }
  ImGui::EndGroup();

  ImGui::SetCursorPosY(y);
  ImGui::SetCursorPosX(x + size.x);
  // draw ouputs
  ImGui::BeginGroup();
  for (std::shared_ptr<ProcessorOutput> output : outputs()) {
    output->draw();
  }
  ImGui::EndGroup();

  ImGui::PopID();
  ImGui::PopStyleVar();
  ImGui::PopStyleVar();

  return m_edit;
}

void chill::Multiplexer::iceSL(std::ofstream& _stream) {
  //write the current Id of the node

  std::string lua = "--[[ " + name() + " ]]--\n";
  lua += "setfenv(1, _G0)  --go back to global initialization\n";
  lua += "__currentNodeId = ";
  lua += std::to_string(reinterpret_cast<int64_t>(this));
  lua += "\n";

  for (auto input : inputs()) {
    // tweak
    if (!input->m_link) {
      lua += "__input[\"" + std::string(input->name()) + "\"] = nil\n";
    }
    // input
    else {
      std::string s2 = std::to_string(reinterpret_cast<int64_t>(input->m_link->owner()));
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
    if (input->m_link) {
      std::string s2 = std::to_string(reinterpret_cast<int64_t>(input->m_link->owner()));
      lua += ", " + s2;
    }
  }

  lua += "})) then\n\
setDirty(__currentNodeId)\n";
  lua += "output('o','UNDEF', input('i', 'UNDEF'))";
  lua += "\nend";

  _stream << lua << std::endl;
}
