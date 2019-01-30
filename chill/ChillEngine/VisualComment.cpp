#include "VisualComment.h"

Chill::VisualComment::VisualComment(VisualComment &copy) {
  m_comment = copy.m_comment;
  m_owner = copy.m_owner;
}


bool Chill::VisualComment::draw() {
  ImGui::PushID(int(getUniqueID()));

  ImDrawList* draw_list = ImGui::GetWindowDrawList();
  ImGuiIO io = ImGui::GetIO();

  ImVec2 size = m_size * m_scale;

  ImVec2 min_pos = ImGui::GetCursorScreenPos();
  ImVec2 max_pos = min_pos + size;

  float border_width = style.processor_border_width * m_scale;
  float rounding_corners = style.processor_rounding_corners * m_scale;


  // border
  ImVec2 border = ImVec2(style.processor_border_width, style.processor_border_width) * m_scale / 2.0f;
  draw_list->AddRect(
    min_pos - border,
    max_pos + border,
    m_selected ?style.processor_selected_color : 0x60FFFFFF,
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
  m_title_size = ImVec2(m_size.x, min(50, m_size.y));
  ImVec2 title_size = m_title_size* m_scale;

  draw_list->AddRectFilled(min_pos, min_pos + title_size,
    0x30FFFFFF,
    rounding_corners, style.processor_rounded_corners & 3);

  if (!m_edit) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 255));
    if (ImGui::ButtonEx((m_comment + "##" + std::to_string(getUniqueID())).c_str(), title_size, ImGuiButtonFlags_PressedOnDoubleClick))
      m_edit = true;
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
  }
  else {
    if (ImGui::InputText(("##" + std::to_string(getUniqueID())).c_str(), m_charComment, 255)) {
      m_comment = m_charComment;
    }
    else if (!m_selected) {
      m_edit = false;
    }
  }

  ImGui::EndGroup();

  return true;
}