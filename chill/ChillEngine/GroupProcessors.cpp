#include "Processor.h"
#include "NodeEditor.h"

namespace Chill
{
  GroupProcessor::GroupProcessor() {
    setName("GroupIO");
    setColor(style.processor_group_color);
  }

  void GroupProcessor::iceSL(std::ofstream& _stream) {
    //write the current Id of the node

    std::string link = "--[[ " + name() + " ]]--\n";
    link += "setfenv(1, _G0)  --go back to global initialization\n";
    link += "__currentNodeId = ";
    link += std::to_string((int64_t)this);
    link += "\n";
    for (auto input : inputs()) {
      // as tweak
      if (input->m_link.isNull()) {
        link += "__input[\"" + std::string(input->name()) + "\"] = " + input->getLuaValue() + "\n";
      }
      // as input
      else {
        std::string s2 = std::to_string((int64_t)input->m_link->owner());
        link += "__input[\"" + std::string(input->name()) + "\"] = " + input->m_link->name() + s2 + "\n";
      }
    }

    //TODO: CLEAN THIS !!!!
    link += "\
_Gcurrent = {} -- clear _Gcurrent\n\
setmetatable(_Gcurrent, { __index = _G0 }) --copy index from _G0\n\
setfenv(1, _Gcurrent)    --set it\n";

    for (auto input : inputs()) {
      link += "output('" + std::string(input->name()) + "', UNDEF, " + input->name() + ")\n" ;
    }

   


    _stream << link << std::endl;
  }

  bool GroupProcessor::draw() {
    ImVec2 initial_pos = ImGui::GetCursorScreenPos();
    Processor::draw();
    ImGui::SetCursorScreenPos(initial_pos);

    ImVec2 title_size(0, style.processor_title_height);
    title_size *= m_scale;

    ImVec2 size = m_size * m_scale;

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

    if (ImGui::BeginDragDropTarget())
    {
      
      if (m_is_output) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_pipe_output")) {
          AutoPtr<ProcessorOutput> output = NodeEditor::Instance()->getSelectedOutput();
          if (output->owner() != this) {
            AutoPtr<ProcessorInput> input = addInput(output->name(), output->type());
            NodeEditor::Instance()->setSelectedInput(input);

            AutoPtr<ProcessorOutput> owner_output = output->clone();
            owner_output->setName(input->name());
            owner()->addOutput(owner_output);
          }
        }
      }

      if (m_is_input) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_pipe_input")) {
          AutoPtr<ProcessorInput> input = NodeEditor::Instance()->getSelectedInput();
          if (input->owner() != this) {
            AutoPtr<ProcessorOutput> output = addOutput(input->name(), input->type());
            NodeEditor::Instance()->setSelectedOutput(output);

            AutoPtr<ProcessorInput> owner_input = input->clone();
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
};