#pragma once

#include "imgui/imgui.h"


struct Style
{
  #define ui_cyan     ImColor( 73, 193, 194) // ImColor(125, 188, 193)
  #define ui_red      ImColor(147,  55,  51)
  #define ui_grey     ImColor(128, 128, 128)
  #define ui_darkgrey ImColor( 38,  38,  38)
  #define ui_orange   ImColor(255, 136,  51)
  #define ui_green    ImColor(104, 194,  73)
  #define ui_yellow   ImColor(255, 255, 100)
  #define ui_white    ImColor(255, 255, 255)
  #define ui_black    ImColor(  0,   0,   0)

  // Window
  ImU32 menubar_color;
  /** Height of the menu bar, needs to be updated before calling "ImGui::EndMainMenuBar();" */
  float menubar_height;

  // Graph
  ImU32 graph_bg_color;
  ImU32 graph_grid_color;

  float graph_grid_line_width;
  int   graph_grid_size;

  // Processor
  ImU32 processor_bg_color;
  ImU32 processor_default_color;
  ImU32 processor_graph_color;
  ImU32 processor_group_color;
  ImU32 processor_selected_color;
  ImU32 processor_error_color;
  ImU32 processor_shadow_color;
  ImU32 processor_shadow_selected_color;

  ImU32 processor_title_bg_color;
  ImU32 processor_title_color;

  float processor_width;
  float processor_title_height;
  float processor_border_width;
  float processor_rounding_corners;

  int   processor_rounded_corners;

  // Socket / Tweak
  float socket_radius;
  float socket_border_width;
  float min_io_height;

  ImVec2 ItemSpacing;

  // Pipe
  ImU32 pipe_color;
  ImU32 pipe_selected_color;
  ImU32 pipe_error_color;
  
  float pipe_line_width;

  Style()
  {
    // Window
    menubar_color  = ui_darkgrey;
    menubar_height = 0.0f;

    // Graph
    graph_bg_color        = ui_darkgrey;
    graph_grid_color      = ui_white & 0x25FFFFFF;
    graph_grid_line_width = 1.0f;
    graph_grid_size       = 10;

    // Processor
    processor_bg_color              = ui_darkgrey;
    processor_default_color         = ui_cyan;
    processor_graph_color           = ui_green;
    processor_group_color           = ui_yellow;
    processor_selected_color        = ui_orange;
    processor_error_color           = ui_red;
    processor_title_color           = ui_darkgrey;
    processor_shadow_color          = ui_black & 0x88FFFFFF;
    processor_shadow_selected_color = ui_orange & 0x88FFFFFF;
    processor_width                 = 220.0f;
    processor_title_height          = 25.0f;
    processor_border_width          = 2.0f;
    processor_rounding_corners      = 5.0f;
    processor_rounded_corners       = (1<<3) + (1<<2) + (1<<1) + 1;

    // Socket / Tweak
    socket_radius       = 10.0f;
    socket_border_width = 0.0f;
    min_io_height       = 50.0f;

    ItemSpacing = ImVec2(5.0f, 5.0f);

    // Pipe
    pipe_color          = ui_cyan;
    pipe_selected_color = ui_orange;
    pipe_error_color    = ImColor(255, 55, 51);

    pipe_line_width = 8.0F;
  };



  ImVec2 delta_to_center_text(ImVec2 element_size, const char *text) {
    ImVec2 text_size = ImGui::CalcTextSize(text);
    return (element_size - text_size) / 2.0f;
  }

  ImVec2 delta_to_right_text(ImVec2 element_size, const char *text) {
    ImVec2 text_size = ImGui::CalcTextSize(text);
    return element_size - text_size;
  }

};
