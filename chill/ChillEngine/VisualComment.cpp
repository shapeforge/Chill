#include "VisualComment.h"

Chill::VisualComment::VisualComment(VisualComment &copy) {
  m_name    = copy.m_name;
  m_color   = copy.m_color;
  m_owner   = copy.m_owner;
  m_comment = copy.m_comment;
}


bool Chill::VisualComment::draw() {

  m_size.y = max(50, m_size.y);
  m_size.x = max(50, m_size.x);
  ImGui::PushID(int(getUniqueID()));

  ImGuiWindow* window = ImGui::GetCurrentWindow();
  ImDrawList* draw_list = ImGui::GetWindowDrawList();

  float w_scale = window->FontWindowScale;
  ImGuiIO io = ImGui::GetIO();

  ImVec2 size = m_size * w_scale;

  ImVec2 min_pos = ImGui::GetCursorScreenPos();
  ImVec2 max_pos = min_pos + size;

  float border_width = style.processor_border_width * w_scale;
  float rounding_corners = style.processor_rounding_corners * w_scale;


  // border
  ImVec2 border = ImVec2(style.processor_border_width, style.processor_border_width) * w_scale / 2.0f;
  draw_list->AddRect(
    min_pos - border,
    max_pos + border,
    m_selected ?style.processor_selected_color : 0x90FFFFFF & m_color,
    rounding_corners, style.processor_rounded_corners,
    border_width
  );

  

  // background
  draw_list->AddRectFilled(min_pos, max_pos, 0x60FFFFFF & m_color, rounding_corners, style.processor_rounded_corners);

  float height = ImGui::GetCursorPosY();

  float padding = (style.socket_radius + style.socket_border_width + style.ItemSpacing.x) * w_scale;

  ImGui::PushItemWidth(m_size.x * w_scale - padding * 2);
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * w_scale);

  ImGui::BeginGroup();
  // draw title
  m_title_size = ImVec2(m_size.x, min(50, m_size.y));
  ImVec2 title_size = m_title_size* w_scale;

  draw_list->AddRectFilled(min_pos, min_pos + title_size,
    0x30FFFFFF,
    rounding_corners, style.processor_rounded_corners & 3);

  if (!m_edit) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 255, 255));
    if (ImGui::ButtonEx((m_name + "##" + std::to_string(getUniqueID())).c_str(), title_size, ImGuiButtonFlags_PressedOnDoubleClick))
      m_edit = true;
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
  }
  else {
    char name[32];
    strcpy(name, m_name.c_str());
    if (ImGui::InputText(("##" + std::to_string(getUniqueID())).c_str(), name, 32)) {
      m_name = name;
    }
    else if (!m_selected) {
      m_edit = false;
    }
  }


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

  ImGui::EndGroup();

  return true;
}