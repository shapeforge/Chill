#include "Processor.h"

#include "NodeEditor.h"

namespace chill {

GroupProcessor::GroupProcessor() {
  setName("GroupIO");
  setColor(style.processor_group_color);
}

//-------------------------------------------------------

void GroupProcessor::iceSL(std::ofstream& _stream) {
  //write the current Id of the node

  std::string code = "--[[ " + name() + " ]]--\n";
  code += "setfenv(1, _G0)  --go back to global initialization\n";
  code += "__currentNodeId = " + std::to_string(reinterpret_cast<int64_t>(this)) + "\n";

  if (owner()->isDirty() || isDirty() || isEmiter()) {
    code += "setDirty(__currentNodeId)\n";
  }

  // GroupInput
  if (!outputs().empty()) {
    for (auto input : owner()->inputs()) {
      // as tweak
      if (!input->m_link) {
        code += "__input['" + std::string(input->name()) + "'] = " + input->getLuaValue() + "\n";
      }
      // as input
      else {
        std::string s2 = std::to_string(reinterpret_cast<int64_t>(input->m_link->owner()));
        code += "__input['" + std::string(input->name()) + "'] = " + input->m_link->name() + s2 + "\n";
      }
    }
  }

  // GroupOutput
  if (!inputs().empty()) {
    for (auto input : inputs()) {
      // as tweak
      if (!input->m_link) {
        code += "__input['" + std::string(input->name()) + "'] = " + input->getLuaValue() + "\n";
      }
      // as input
      else {
        std::string s2 = std::to_string(reinterpret_cast<int64_t>(input->m_link->owner()));
        code += "__input['" + std::string(input->name()) + "'] = " + input->m_link->name() + s2 + "\n";
      }
    }
  }

  //TODO: CLEAN THIS !!!!
  code += "\
      _Gcurrent = {} -- clear _Gcurrent\n\
      setmetatable(_Gcurrent, { __index = _G0 }) --copy index from _G0\n\
      setfenv(1, _Gcurrent)    --set it\n"
      ;

  // GroupInput
  if (!outputs().empty()) {
    for (auto output : outputs()) {
      code += "output('" + std::string(output->name()) + "', 'UNDEF', input('" + output->name() + "'))\n";
    }
  }

  // GroupOutput
  if (!inputs().empty()) {
    for (auto output : owner()->outputs()) {
      code += std::string(output->name()) + " = input('" + output->name() + "')\n";
      // set the parent as current node
      code += "setNodeId(" + std::to_string(reinterpret_cast<int64_t>(owner())) + ")\n";
      code += "output('" + std::string(output->name()) + "', 'UNDEF', " + output->name() + ")\n";
      // reset current node
      code += "setNodeId(" + std::to_string(reinterpret_cast<int64_t>(this)) + ")\n";
    }
  }

  _stream << code << std::endl;
}

bool GroupProcessor::draw() {
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  float w_scale = window->FontWindowScale;

  ImVec2 initial_pos = ImGui::GetCursorScreenPos();
  Processor::draw();
  ImGui::SetCursorScreenPos(initial_pos);

  ImVec2 title_size(0, style.processor_title_height);
  title_size *= w_scale;

  ImVec2 size = m_size * w_scale;

  ImVec2 min_pos = initial_pos;
  ImGui::SetCursorScreenPos(min_pos);
  ImVec2 max_pos = min_pos + size;

  // draw dropzone
  ImGui::PushID(int(getUniqueID()));

  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0x00000000);
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0x00000000);
  ImGui::ButtonEx("", max_pos - min_pos, ImGuiButtonFlags_PressedOnDoubleClick);
  ImGui::PopStyleColor();
  ImGui::PopStyleColor();

  if (ImGui::BeginDragDropTarget()) {
    if (m_is_output) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_pipe_output")) {
        std::shared_ptr<ProcessorOutput> output = NodeEditor::Instance()->getSelectedOutput();
        if (output->owner() != this) {
          std::shared_ptr<ProcessorInput> input = addInput(output->name(), output->type());
          NodeEditor::Instance()->setSelectedInput(input);

          std::shared_ptr<ProcessorOutput> owner_output = output->clone();
          owner_output->setName(input->name());
          owner()->addOutput(owner_output);
        }
      }
    }

    if (m_is_input) {
      if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_pipe_input")) {
        std::shared_ptr<ProcessorInput> input = NodeEditor::Instance()->getSelectedInput();
        if (input->owner() != this) {
          std::shared_ptr<ProcessorOutput> output = addOutput(input->name(), input->type());
          NodeEditor::Instance()->setSelectedOutput(output);

          std::shared_ptr<ProcessorInput> owner_input = input->clone();
          owner_input->setName(output->name());
          owner()->addInput(owner_input);
        }
      }
    }
    ImGui::EndDragDropTarget();
  }
  ImGui::PopID();
  return true;
}

} // namespace chill
