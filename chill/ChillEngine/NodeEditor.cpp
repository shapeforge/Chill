#include "NodeEditor.h"
#include "Processor.h"
#include "LuaProcessor.h"
#include "IOs.h"
#include "GraphSaver.h"
#include "FileDialog.h"
#include "Resources.h"
#include "VisualComment.h"

#ifdef USE_GLUT
#include <GL/glut.h>
#endif

#ifdef WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

LIBSL_WIN32_FIX

namespace Chill
{
  NodeEditor* NodeEditor::s_instance = nullptr;

  //-------------------------------------------------------
  void listFolderinDir(std::vector<std::string>& files, std::string folder)
  {
    for (fs::directory_iterator itr(folder); itr != fs::directory_iterator(); ++itr)
    {
      fs::path file = itr->path();
      if (is_directory(file))files.push_back(file.filename().generic_string());
    }
  }

  void listLuaFileInDir(std::vector<std::string>& files)
  {
    listFiles(NodeEditor::NodesFolder().c_str(), files);
  }

  void listLuaFileInDir(std::vector<std::string>& files, std::string directory)
  {
    for (fs::directory_iterator itr(directory); itr != fs::directory_iterator(); ++itr)
    {
      fs::path file = itr->path();
      if (!is_directory(file) && strcmp(extractExtension(file.filename().generic_string()).c_str(), "lua") == 0) {
        files.push_back(file.filename().generic_string());
      }
    }
  }

  //---------------------------------------------------
  std::string recursiveFileSelecter(std::string current_dir)
  {
    std::vector<std::string> files;
    listLuaFileInDir(files, current_dir);
    std::vector<std::string> directories;
    std::string nameDir = "";
    listFolderinDir(directories, current_dir);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));
    ForIndex(i, directories.size()) {
      if (ImGui::BeginMenu(directories[i].c_str())) {
        nameDir = recursiveFileSelecter(Resources::toPath(current_dir, directories[i]));
        ImGui::EndMenu();
      }
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1., 1., 1.0, 1));
    ForIndex(i, files.size()) {
      if (ImGui::MenuItem(removeExtensionFromFileName(files[i]).c_str())) {
        nameDir = Resources::toPath(current_dir, files[i]);
      }
    }
    ImGui::PopStyleColor();

    return nameDir;
  }

  //-------------------------------------------------------
  std::string relativePath(std::string& path)
  {
    int nfsize = (int)NodeEditor::NodesFolder().size();
    if (path[nfsize + 1] == Resources::separator()) nfsize++;
    std::string name = path.substr(nfsize);
    return name;
  }

  //-------------------------------------------------------
  void addNodeMenu(ImVec2 pos) {
    NodeEditor* n_e = NodeEditor::Instance();
    std::string node = recursiveFileSelecter(NodeEditor::NodesFolder());
    if (!node.empty()) {
      AutoPtr<LuaProcessor> proc = n_e->getCurrentGraph()->addProcessor<LuaProcessor>(relativePath(node));
      proc->setPosition(pos);
    }
  }

  //-------------------------------------------------------
#ifdef WIN32
  BOOL CALLBACK EnumWindowsFromPid(HWND hwnd, LPARAM lParam)
  {
    DWORD pID;
    GetWindowThreadProcessId(hwnd, &pID);
    if (pID == lParam)
    {
      NodeEditor::Instance()->g_icesl_hwnd = hwnd;
      return FALSE;
    }
    return TRUE;
  }

  BOOL CALLBACK TerminateAppEnum(HWND hwnd, LPARAM lParam)
  {
    DWORD pID;
    GetWindowThreadProcessId(hwnd, &pID);

    if (pID == (DWORD)lParam)
    {
      PostMessage(hwnd, WM_CLOSE, 0, 0);
    }

    return TRUE;
  }

  LRESULT CALLBACK custom_wndProc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
  {
    switch (uMsg) {
    case WM_MOVE:
      NodeEditor::Instance()->moveIceSLWindowAlongChill();

      // get window placement / style
      WINDOWPLACEMENT wPlacement;
      GetWindowPlacement(NodeEditor::Instance()->g_chill_hwnd, &wPlacement);

      if (wPlacement.showCmd == SW_MAXIMIZE) {
        // set default pos
        NodeEditor::Instance()->setDefaultAppsPos();
      }
      break;
    }
    return 0;
  }
#endif

  //-------------------------------------------------------
  NodeEditor::NodeEditor() {
    m_graphs.push(new ProcessingGraph());
  }

  //-------------------------------------------------------
  void NodeEditor::mainRender()
  {
    glClearColor(0.F, 0.F, 0.F, 0.F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Instance()->draw();

    ImGui::Render();
  }

  //-------------------------------------------------------
  void NodeEditor::mainOnResize(uint width, uint height) {
    Instance()->m_size = ImVec2((float)width, (float)height);
    s_instance->moveIceSLWindowAlongChill();
  }

  //-------------------------------------------------------
  void NodeEditor::mainKeyPressed(uchar k)
  {
  }

  //-------------------------------------------------------
  void NodeEditor::mainScanCodePressed(uint sc)
  {
    if (sc == LIBSL_KEY_SHIFT) {
      ImGui::GetIO().KeyShift = true;
    }
    if (sc == LIBSL_KEY_CTRL) {
      ImGui::GetIO().KeyCtrl = true;
    }
  }

  //-------------------------------------------------------
  void NodeEditor::mainScanCodeUnpressed(uint sc)
  {
    if (sc == LIBSL_KEY_SHIFT) {
      ImGui::GetIO().KeyShift = false;
    }
    if (sc == LIBSL_KEY_CTRL) {
      ImGui::GetIO().KeyCtrl = false;
    }
  }

  //-------------------------------------------------------
  void NodeEditor::mainMouseMoved(uint x, uint y) {}

  //-------------------------------------------------------
  void NodeEditor::mainMousePressed(uint x, uint y, uint button, uint flags) {}

  //-------------------------------------------------------
  bool NodeEditor::draw()
  {
    drawMenuBar();

    ImGui::SetNextWindowPos(ImVec2(0, 20));
    ImGui::SetNextWindowSize(ImVec2(200, m_size.y));
    drawLeftMenu();

    ImGui::SetNextWindowPos(ImVec2(200, 20));
    ImGui::SetNextWindowSize(m_size);
    drawGraph();

    return true;
  }

  //-------------------------------------------------------
  void NodeEditor::drawMenuBar()
  {
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, style.menubar_color);

    if (ImGui::BeginMainMenuBar()) {
      // opening file
      std::string fullpath = "";
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New graph")) {
          std::string graph_filename = getMainGraph()->name() + ".graph";
          fullpath = saveFileDialog(graph_filename.c_str(), OFD_FILTER_GRAPHS);
          if (!fullpath.empty()) {
            std::ofstream file;
            file.open(fullpath);
            setMainGraph(new ProcessingGraph());
            getMainGraph()->save(file);
            file.close();
            g_graphPath = fullpath;
          }
        }
        if (ImGui::MenuItem("Load graph")) {
          fullpath = openFileDialog(OFD_FILTER_GRAPHS);
          if (!fullpath.empty()) {
            GraphSaver *test = new GraphSaver();
            test->execute(fullpath.c_str());
            delete test;
            g_graphPath = fullpath;
          }
        }
        if (ImGui::MenuItem("Save graph", "Ctrl+S")) {
          std::string graph_filename = getMainGraph()->name() + ".graph";
          fullpath = saveFileDialog(graph_filename.c_str(), OFD_FILTER_GRAPHS);
          if (!fullpath.empty()) {
            std::ofstream file;
            file.open(fullpath);
            getMainGraph()->save(file);
            file.close();
            g_graphPath = fullpath;
          }
        }
        if (ImGui::MenuItem("Save current graph", "Ctrl+Shift+S")) {
          std::string graph_filename = getCurrentGraph()->name() + ".graph";
          fullpath = saveFileDialog(graph_filename.c_str(), OFD_FILTER_GRAPHS);
          if (!fullpath.empty()) {
            std::ofstream file;
            file.open(fullpath);
            getCurrentGraph()->save(file);
            file.close();
          }
        }

        if (!fullpath.empty()) {
          const size_t last_slash_idx = fullpath.rfind(Resources::separator());
          if (std::string::npos != last_slash_idx) {
            //m_nodeFolder = fullpath.substr(0, last_slash_idx) + Resources::separator() + "nodes";
          }
        }


        if (ImGui::MenuItem("Export to IceSL lua")) {
          std::string graph_filename = getMainGraph()->name() + ".lua";
          g_iceSLExportPath = saveFileDialog(graph_filename.c_str(), OFD_FILTER_LUA);
          exportIceSL(g_iceSLExportPath);
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Settings")) {
        ImGui::MenuItem("Automatic save", "", &g_auto_save);
        ImGui::MenuItem("Automatic export", "", &g_auto_export);
        ImGui::MenuItem("Automatic use of IceSL", "", &g_auto_icesl);
        ImGui::EndMenu();
      }
      // save the effective menubar height in the style
      style.menubar_height = ImGui::GetWindowSize().y;
      ImGui::EndMainMenuBar();
    }

    ImGui::PopStyleColor();
  }

  //-------------------------------------------------------

  std::vector<AutoPtr<SelectableUI>>     hovered;
  std::vector<AutoPtr<SelectableUI>>     selected;
  AutoPtr<ProcessingGraph> buffer;


  void NodeEditor::drawLeftMenu()
  {
    char title[32];
    char name[32];

    ImGui::Begin("GraphInfo", &m_visible,
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse |

      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoScrollWithMouse |

      ImGuiWindowFlags_NoBringToFrontOnFocus
    );


    if (ImGui::CollapsingHeader("Graph tree")) {
      uint i = 1;

      ImVec2 size(100, 20);

#if WIN32
      //TODO _Get_container is not standard
      for (auto graph : m_graphs._Get_container()) {
        std::string text = "";
        for (int j = 0; j < i; j++)
          text += " ";
        text += ">";
        ImGui::TextDisabled(text.c_str());
        ImGui::SameLine();

        if (i == m_graphs.size()) {
          strcpy(title, m_graphs.top()->name().c_str());
          if (ImGui::InputText(("##graphname" + std::to_string(getUniqueID())).c_str(), title, 32)) {
            m_graphs.top()->setName(title);
          }
        } else {
          if (ImGui::Button(graph->name().c_str())) {
            while (m_graphs.size() > i) {
              m_graphs.pop();
            }
          }
        }

        i++;
      }
#endif
    }

    ImGui::NewLine();
    if (selected.size() == 1) {
      ImGui::Text("Name:");
      strcpy(name, selected[0]->name().c_str());
      if (ImGui::InputText(("##" + std::to_string(getUniqueID())).c_str(), name, 16)) {
        m_graphs.top()->setName(name);
      }

      ImGuiColorEditFlags flags = ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaBar;
      ImVec4 col = ImGui::ColorConvertU32ToFloat4(selected[0]->color());
      float color[4] = { col.x, col.y , col.z , col.w };
      if (ImGui::ColorPicker4("Color", color, flags)) {
        selected[0]->setColor(ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3])));
      }
    }

    ImGui::End();
  }

  //-------------------------------------------------------
  void NodeEditor::drawGraph()
  {
    linking = !m_selected_input.isNull() || !m_selected_output.isNull();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, style.graph_bg_color);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("Graph", &m_visible,
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse |

      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoScrollWithMouse |

      ImGuiWindowFlags_NoBringToFrontOnFocus
    );

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImDrawList* overlay_draw_list = ImGui::GetOverlayDrawList();

    ImVec2 w_pos = window->Pos;
    ImVec2 w_size = window->Size;
    float w_scale = window->FontWindowScale;

    ImVec2 offset = (m_offset * w_scale + (w_size - w_pos) / 2.0F);

    ImGuiIO io = ImGui::GetIO();

    for (AutoPtr<Processor> processor : m_graphs.top()->processors()) {
      if (processor->isDirty()) {
        dirty = true;
        break;
      }
    }

    if (dirty) {
      if (g_auto_export) {
#ifdef WIN32
        exportIceSL(g_iceSLTempExportPath);
#endif
        exportIceSL(g_iceSLExportPath);
      }

      if (g_auto_save) {
        std::ofstream file;
        file.open(g_graphPath);
        m_graphs.top()->save(file);
      }
      dirty = false;
    }

    if (ImGui::IsWindowHovered()) {
      hovered.clear();
      selected.clear();
      text_editing = false;

      for (AutoPtr<Processor> processor : m_graphs.top()->processors()) {
        ImVec2 socket_size = ImVec2(1, 1) * (style.socket_radius + style.socket_border_width) * w_scale;

        ImVec2 size = processor->m_size * window->FontWindowScale;
        ImVec2 min_pos = offset + w_pos + processor->m_position * w_scale - socket_size;
        ImVec2 max_pos = min_pos + size + socket_size * 2;

        if (ImGui::IsMouseHoveringRect(min_pos, max_pos)) {
          hovered.push_back(AutoPtr<SelectableUI>(processor));
        }

        if (processor->m_selected) {
          selected.push_back(AutoPtr<SelectableUI>(processor));
        }

        if (processor->m_edit) {
          text_editing = true;
        }
      }

      for (AutoPtr<VisualComment> comment : m_graphs.top()->comments()) {
        ImVec2 socket_size = ImVec2(1, 1) * (style.socket_radius + style.socket_border_width) * w_scale;

        ImVec2 size = comment->m_title_size * w_scale;
        ImVec2 min_pos = offset + w_pos + comment->m_position * w_scale - socket_size;
        ImVec2 max_pos = min_pos + size + socket_size * 2;

        if (ImGui::IsMouseHoveringRect(min_pos, max_pos)) {
          hovered.push_back(AutoPtr<SelectableUI>(comment));
        }

        if (comment->m_selected) {
          selected.push_back(AutoPtr<SelectableUI>(comment));
        }

        if (comment->m_edit) {
          text_editing = true;
        }
      }

      /*
      if (selected_processors.empty()) {
        for (AutoPtr<VisualComment> comment : m_graphs.top()->comments()) {
          ImVec2 size = comment->m_size * comment->m_scale;
          ImVec2 min_pos = offset + win_pos + comment->m_position * m_scale;
          ImVec2 max_pos = min_pos + size;
          if (ImGui::IsMouseHoveringRect(min_pos, max_pos)) {
            hovered_comments.push_back(comment);
          }
        }
      }*/

      // LEFT CLICK
      if (linking) {
        m_selecting = false;
        m_dragging = false;

        if (io.MouseDown[0] && hovered.empty() || io.MouseDown[1]) {
          linking = false;
          m_selected_input = AutoPtr<ProcessorInput>(NULL);
          m_selected_output = AutoPtr<ProcessorOutput>(NULL);
        }
      }
      else {
        if (io.MouseDoubleClicked[0]) {
          for (AutoPtr<SelectableUI> hovproc : hovered) {
            if (ProcessingGraph* v = dynamic_cast<ProcessingGraph*>(hovproc.raw())) {
              if (!v->m_edit) {
                m_graphs.push(v);
                m_offset = m_graphs.top()->getBarycenter() * -1.0F;
                break;
              }
            }
          }
        }

        if (io.MouseDown[0] && io.MouseDownOwned[0]) {
          if (!hovered.empty() && !m_selecting) {
            for (AutoPtr<SelectableUI> selproc : selected) {
              for (AutoPtr<SelectableUI> hovproc : hovered) {
                if (hovproc == selproc) {
                  m_dragging = true;
                }
              }
            }
          }

          if (hovered.empty() && !m_dragging || m_selecting) {
            selectProcessors();
          }
        }
        else {
          m_selecting = false;
          m_dragging = false;
        }

        if (io.MouseClicked[0]) {
          // if no processor hovered, clear
          if (!io.KeysDown[LIBSL_KEY_SHIFT] && hovered.empty()) {
            for (AutoPtr<SelectableUI> selproc : selected) {
              selproc->m_selected = false;
            }
            selected.clear();
          }
          else {


            for (AutoPtr<SelectableUI> hovproc : hovered) {
              if (io.KeysDown[LIBSL_KEY_SHIFT]) {
                if (hovproc->m_selected) {
                  hovproc->m_selected = false;
                  selected.erase(std::find(selected.begin(), selected.end(), hovproc));
                }
                else {
                  hovproc->m_selected = true;
                  selected.push_back(hovproc);
                }
              }
              else {
                bool sel_and_hov = false;
                for (AutoPtr<SelectableUI> selproc : selected) {
                  if (selproc == hovproc) {
                    sel_and_hov = true;
                    break;
                  }
                }
                if (!sel_and_hov) {
                  for (AutoPtr<SelectableUI> selproc : selected) {
                    selproc->m_selected = false;
                  }
                  selected.clear();

                  hovproc->m_selected = true;
                  selected.push_back(hovproc);
                }
              }
            }
          }
        }

      } // ! LEFT CLICK

      { // RIGHT CLICK
        if (io.MouseClicked[1]) {
          if (selected.empty() || (selected.size() == 1 && hovered.empty())) {
            m_graph_menu = true;
          }
          else {
            m_node_menu = true;
          }
        }
      } // ! RIGHT CLICK

      // Move selected processors
      if (m_dragging) {
        if (!io.KeysDown[LIBSL_KEY_CTRL]) {
          for (AutoPtr<SelectableUI> selected : selected) {
            selected->translate(io.MouseDelta / w_scale);
          }
        }
        else {
          for (AutoPtr<SelectableUI> selected : selected) {
            AutoPtr<VisualComment> com(selected);
            if (!com.isNull())
              selected->m_size += io.MouseDelta / w_scale;
          }
        }
      }

      // Move the whole canvas
      if (io.MouseDown[2] && ImGui::IsMouseHoveringAnyWindow()) {
        m_offset += io.MouseDelta / w_scale;
      }

      zoom();

      // Keyboard
      if (!text_editing)
      {
        if (io.KeysDown['g'] && io.KeysDownDuration['g'] == 0) {
          if (!selected.empty()) {
            getCurrentGraph()->collapseSubset(selected);
            selected.clear();
          }
        }

        if (io.KeysDown['b'] && io.KeysDownDuration['b'] == 0) {
          if (m_graphs.size() > 1) {
            m_graphs.pop();
            m_offset = getCurrentGraph()->getBarycenter() * -1.0F;
          }
        }
        if (io.KeysDown['c'] && io.KeysDownDuration['c'] == 0) {
          // mouse to screen
          ImVec2 m2s = io.MousePos - (w_pos + w_size) / 2.0F;
          // screen to grid
          ImVec2 s2g = m2s / w_scale - m_offset;
          AutoPtr<VisualComment> com(new VisualComment());
          com->setPosition(s2g);
          this->getCurrentGraph()->addComment(com);
        }
      }

      if (io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['c'] && io.KeysDownDuration['c'] == 0) {
        if (!selected.empty()) {
          buffer = getCurrentGraph()->copySubset(selected);
        }
      }
      if (!buffer.isNull()) {
        if (ImGui::MenuItem("Paste", "CTRL+V")) {
          //getCurrentGraph()->expandGraph(buffer, s2g);
          buffer = AutoPtr<ProcessingGraph>(NULL);
        }
      }
      if (io.KeysDown[LIBSL_KEY_DELETE]) {
        for (AutoPtr<SelectableUI> item : selected) {
          getCurrentGraph()->remove(item);
        }
      }

      // update the offset used for display
      offset = (m_offset * w_scale + (w_size - w_pos) / 2.0F);
    } // ! if window hovered

    // Draw the grid
    if (m_show_grid)
    {
      drawGrid();
    }
    ProcessingGraph* currentGraph = m_graphs.top();

    for (AutoPtr<VisualComment> comment : currentGraph->comments()) {
      ImVec2 position = offset + comment->m_position * w_scale;
      ImGui::SetCursorPos(position);
      comment->draw();
    }

    // Draw the nodes
    for (AutoPtr<Processor> processor : currentGraph->processors()) {
      ImVec2 position = offset + processor->m_position * w_scale;
      ImGui::SetCursorPos(position);
      processor->draw();
    }

    // Draw the pipes
    float pipe_width = 2.0F * w_scale;
    int pipe_res = int(25.0F / std::abs(std::log(w_scale / 2.0F)));
    for (AutoPtr<Processor> processor : currentGraph->processors()) {
      for (AutoPtr<ProcessorOutput> output : processor->outputs()) {
        for (AutoPtr<ProcessorInput> input : output->m_links) {
          ImVec2 A = input->getPosition() + w_pos;
          ImVec2 B = output->getPosition() + w_pos;

          ImVec2 bezier(abs(A.x - B.x) / 2.0F * w_scale, 0.0F);

          ImGui::GetWindowDrawList()->AddBezierCurve(
            A,
            A - bezier,
            B + bezier,
            B,
            processor->color(),
            pipe_width,
            pipe_res
          );
        }
      }
    }

    // Draw new pipe
    if (linking) {
      float pipe_width = 2.0F * w_scale;
      int pipe_res = int(25.0F / std::abs(std::log(w_scale / 2.0F)));

      ImVec2 A = ImGui::GetMousePos();
      ImVec2 B = ImGui::GetMousePos();

      if (!m_selected_input.isNull()) {
        A = w_pos + m_selected_input->getPosition();
      }
      else if (!m_selected_output.isNull()) {
        B = w_pos + m_selected_output->getPosition();
      }

      ImVec2 bezier(100.0f * w_scale, 0);
      ImGui::GetWindowDrawList()->AddBezierCurve(
        A,
        A - bezier,
        B + bezier,
        B,
        style.pipe_selected_color,
        pipe_width,
        pipe_res
      );
    }

    // Draw graph nav
    ImGui::SetCursorPos(w_pos);
    uint i = 1;

    ImVec2 pos = w_pos;
    ImVec2 size(100, 20);

    size *= w_scale;

    for (auto graph : m_graphs._Get_container()) {
      draw_list->AddRectFilled(pos, pos + size, style.processor_bg_color, 0, 0);
      if (ImGui::IsMouseHoveringRect(pos, pos + size)) {
        draw_list->AddRectFilled(pos, pos + size, style.processor_bg_color, 0, 0);

        if (ImGui::IsMouseClicked(0)) {
          while (m_graphs.size() > i) {
            m_graphs.pop();
          }
        }
      }
      else {
        draw_list->AddRectFilled(pos, pos + size, style.processor_bg_color, 0, 0);
      }

      ImGui::SetCursorPos(pos - w_pos + style.delta_to_center_text(ImVec2(10, size.y), ">"));
      ImGui::Text(">");
      ImGui::SetCursorPos(pos - w_pos + style.delta_to_center_text(size, graph->name().c_str()));
      ImGui::Text(graph->name().c_str());

      pos.x += size.x;
      i++;
    }

    menus();

    ImGui::End(); // Graph

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
  }

  //-------------------------------------------------------
  void NodeEditor::zoom() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO io = ImGui::GetIO();

    if (true || io.FontAllowUserScaling) {
      float old_scale = window->FontWindowScale;
      float new_scale = ImClamp(window->FontWindowScale + io.MouseWheel * 0.1F, 0.3F, 2.0F);
      float scale = new_scale / old_scale;
      window->FontWindowScale = new_scale;

      const ImVec2 offset = window->Size * (1.F - scale) * (io.MousePos - window->Pos) / window->Size;

      if (new_scale != old_scale) {
        // mouse to screen
        ImVec2 m2s = io.MousePos - (window->Pos + window->Size) / 2.F;
        // screen to grid
        ImVec2 s2g = m2s / old_scale - m_offset;
        // grid to screen
        ImVec2 g2s = (s2g + m_offset) * new_scale;

        m_offset += (m2s - g2s) / new_scale;
      }
    }
  }

  //-------------------------------------------------------
  void NodeEditor::drawGrid() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImVec2 offset = (NodeEditor::Instance()->m_offset * window->FontWindowScale + (window->Size - window->Pos) / 2.F);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const ImU32& GRID_COLOR = NodeEditor::Instance()->style.graph_grid_color;
    const float grid_Line_width = NodeEditor::Instance()->style.graph_grid_line_width;

    int coeff = window->FontWindowScale >= 0.5F ? 1 : 10;
    int subdiv_levels[] = {
      NodeEditor::Instance()->style.graph_grid_size * coeff,
      NodeEditor::Instance()->style.graph_grid_size * coeff * 10 };

    for (int subdiv : subdiv_levels) {
      const float& grid_size = window->FontWindowScale * subdiv;

      // Vertical lines
      for (float x = fmodf(offset.x, grid_size); x < window->Size.x; x += grid_size) {
        ImVec2 p1 = ImVec2(x + window->Pos.x, window->Pos.y);
        ImVec2 p2 = ImVec2(x + window->Pos.x, window->Size.y + window->Pos.y);
        draw_list->AddLine(p1, p2, GRID_COLOR, grid_Line_width);
      }

      // Horizontal lines
      for (float y = fmodf(offset.y, grid_size); y < window->Size.y; y += grid_size) {
        ImVec2 p1 = ImVec2(window->Pos.x, y + window->Pos.y);
        ImVec2 p2 = ImVec2(window->Size.x + window->Pos.x, y + window->Pos.y);
        draw_list->AddLine(p1, p2, GRID_COLOR, grid_Line_width);
      }
    }
  }

  //-------------------------------------------------------
  void NodeEditor::menus() {
    NodeEditor* n_e = NodeEditor::Instance();

    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImVec2 w_pos = window->Pos;
    ImVec2 w_size = window->Size;
    float w_scale = window->FontWindowScale;

    ImGuiIO io = ImGui::GetIO();

    // Draw menus
    if (NodeEditor::Instance()->m_graph_menu) {
      ImGui::OpenPopup("graph_menu");
    }
    if (NodeEditor::Instance()->m_node_menu) {
      ImGui::OpenPopup("node_menu");
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
    if (ImGui::BeginPopup("graph_menu"))
    {
      // mouse to screen
      ImVec2 m2s = io.MousePos - (w_pos + w_size) / 2.0F;
      // screen to grid
      ImVec2 s2g = m2s / w_scale - NodeEditor::Instance()->m_offset;

      NodeEditor::Instance()->m_graph_menu = false;

      if (ImGui::BeginMenu("Add", "SPACE")) {
        addNodeMenu(s2g);
        ImGui::EndMenu();
      }

      if (!buffer.isNull()) {
        if (ImGui::MenuItem("Paste", "CTRL+V")) {
          n_e->getCurrentGraph()->expandGraph(buffer, s2g);
          buffer = AutoPtr<ProcessingGraph>(NULL);
        }
      }

      if (ImGui::MenuItem("Add multiplexer node")) {
        AutoPtr<Multiplexer> proc = n_e->getCurrentGraph()->addProcessor<Multiplexer>();
        proc->setPosition(s2g);
      }

      if (ImGui::MenuItem("Comment")) {
        AutoPtr<VisualComment> com(new VisualComment());
        com->setPosition(s2g);
        n_e->getCurrentGraph()->addComment(com);
      }

      if (ImGui::MenuItem("Nothing")) {

      }


      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("node_menu"))
    {
      NodeEditor::Instance()->m_node_menu = false;
      if (ImGui::MenuItem("Copy", "CTRL+C")) {
        if (!selected.empty()) {
          buffer = n_e->getCurrentGraph()->copySubset(selected);
        }
      }
      if (ImGui::MenuItem("Group")) {
        if (!selected.empty()) {
          n_e->getCurrentGraph()->collapseSubset(selected);
        }
      }
      if (ImGui::MenuItem("Ungroup")) {
        if (!selected.empty()) {
          for (AutoPtr<SelectableUI> proc : selected) {
            if (typeid(*proc.raw()) == typeid(ProcessingGraph)) {
              n_e->getCurrentGraph()->expandGraph(AutoPtr<ProcessingGraph>(proc), proc->getPosition());
            }
          }
        }
      }

      if (ImGui::MenuItem("Delete")) {
        for (AutoPtr<SelectableUI> item : selected) {
          n_e->getCurrentGraph()->remove(item);
        }
      }
      if (ImGui::MenuItem("Unlink")) {
        for (AutoPtr<SelectableUI> select : selected) {
          AutoPtr<Processor> proc = AutoPtr<Processor>(select);
          if (!proc.isNull()) {
            for (AutoPtr<ProcessorInput> input : proc->inputs())
            {
              n_e->getCurrentGraph()->disconnect(input);
            }
            for (AutoPtr<ProcessorOutput> output : proc->outputs()) {
              for (AutoPtr<ProcessorInput> input : output->m_links) {
                n_e->getCurrentGraph()->disconnect(input);
              }
            }
          }
        }
      }
      if (ImGui::MenuItem("Nothing")) {}
      ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
  }

  //-------------------------------------------------------
  void NodeEditor::selectProcessors() {
    NodeEditor::Instance()->m_selecting = true;

    NodeEditor* n_e = NodeEditor::Instance();
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2  w_pos = window->Pos;
    ImVec2 w_size = window->Size;
    float w_scale = window->FontWindowScale;

    ImGuiIO io = ImGui::GetIO();

    ImVec2 A = (io.MousePos - (w_pos + w_size) / 2.0F) / w_scale - NodeEditor::Instance()->m_offset;
    ImVec2 B = (io.MouseClickedPos[0] - (w_pos + w_size) / 2.0F) / w_scale - NodeEditor::Instance()->m_offset;

    if (A.x > B.x) std::swap(A.x, B.x);
    if (A.y > B.y) std::swap(A.y, B.y);

    if (B.x - A.x > 10 || B.y - A.y > 10) {
      draw_list->AddRect(
        io.MouseClickedPos[0],
        io.MousePos,
        NodeEditor::Instance()->style.processor_selected_color
      );

      if (!io.KeysDown[LIBSL_KEY_SHIFT]) {
        for (AutoPtr<SelectableUI> selproc : selected) {
          selproc->m_selected = false;
        }
        selected.clear();
      }

      for (AutoPtr<Processor> procui : n_e->getCurrentGraph()->processors()) {
        ImVec2 pos_min = procui->getPosition();
        ImVec2 pos_max = pos_min + procui->m_size;

        if (pos_min.x < A.x || B.x < pos_max.x) continue;
        if (pos_min.y < A.y || B.y < pos_max.y) continue;
        procui->m_selected = true;
        selected.push_back(AutoPtr<SelectableUI>(procui));
      }

      for (AutoPtr<VisualComment> comui : n_e->getCurrentGraph()->comments()) {
        ImVec2 pos_min = comui->getPosition();
        ImVec2 pos_max = pos_min + comui->m_size;

        if (pos_min.x < A.x || B.x < pos_max.x) continue;
        if (pos_min.y < A.y || B.y < pos_max.y) continue;
        comui->m_selected = true;
        selected.push_back(AutoPtr<SelectableUI>(comui));
      }
    }
  }

  //-------------------------------------------------------
  void NodeEditor::launchIcesl() {
#ifdef WIN32
    // CreateProcess init
    const char* icesl_path = s_instance->g_iceslPath.c_str();
    std::string icesl_params = "--remote " + s_instance->g_iceSLTempExportPath;

    STARTUPINFO StartupInfo;
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof StartupInfo;

    ZeroMemory(&s_instance->g_icesl_p_info, sizeof(s_instance->g_icesl_p_info));

    // create the process
    auto icesl_process = CreateProcess(icesl_path, // @lpApplicationName - application name / path 
      LPSTR(icesl_params.c_str()), // @lpCommandLine - command line for the application
      NULL, // @lpProcessAttributes - SECURITY_ATTRIBUTES for the process
      NULL, // @lpThreadAttributes - SECURITY_ATTRIBUTES for the thread
      FALSE, // @bInheritHandles - inherit handles ?
      CREATE_NEW_CONSOLE, // @dwCreationFlags - process creation flags
      NULL, // @lpEnvironment - environment
      NULL, // @lpCurrentDirectory - current directory
      &StartupInfo, // @lpStartupInfo -startup info
      &s_instance->g_icesl_p_info); // @lpProcessInformation - process info

    if (icesl_process)
    {
      // watch the process
      WaitForSingleObject(s_instance->g_icesl_p_info.hProcess, 1000);
      // getting the hwnd
      EnumWindows(EnumWindowsFromPid, s_instance->g_icesl_p_info.dwProcessId);
    }
    else
    {
      // process creation failed
      std::cerr << Console::red << "Icesl couldn't be opened, please launch Icesl manually" << Console::gray << std::endl;
      std::cerr << Console::red << "ErrorCode: " << GetLastError() << Console::gray << std::endl;

      s_instance->g_icesl_hwnd = NULL;
    }
#endif
  }

  //-------------------------------------------------------
  void NodeEditor::closeIcesl() {
#ifdef WIN32
    if (s_instance->g_icesl_p_info.hProcess == NULL) {
      std::cerr << Console::red << "Icesl Handle not found. Please close Icesl manually." << Console::gray << std::endl;
    }
    else {
      // close all windows with icesl's pid
      EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)s_instance->g_icesl_p_info.dwProcessId);

      // check if handle still responds, else handle is killed
      if (WaitForSingleObject(s_instance->g_icesl_p_info.hProcess, 1000) != WAIT_OBJECT_0) {
        TerminateProcess(s_instance->g_icesl_p_info.hProcess, 0);
        std::cerr << Console::yellow << "Trying to force close Icesl." << Console::gray << std::endl;
      }
      else {
        std::cerr << Console::yellow << "Icesl was successfully closed." << Console::gray << std::endl;
      }
      CloseHandle(s_instance->g_icesl_p_info.hProcess);
    }
#endif
  }

  //-------------------------------------------------------
  void NodeEditor::exportIceSL(std::string& filename) {
    if (!filename.empty()) {
      std::ofstream file;
      file.open(filename);

      // TODO: CLEAN THIS !!!

      file <<
        "enable_variable_cache = false" << std::endl <<
        std::endl <<
        "local _G0 = {}       --swap environnement(swap variables between scripts)" << std::endl <<
        "local _Gcurrent = {} --environment local to the script : _Gc includes _G0" << std::endl <<
        "local __dirty = {}   --table of all dirty nodes" << std::endl <<
        "__input = {}         --table of all input values" << std::endl <<
        "" << std::endl <<
        "setmetatable(_G0, { __index = _G })" << std::endl <<
        "" << std::endl <<
        "function setNodeId(id)" << std::endl <<
        "  setfenv(1, _G0)" << std::endl <<
        "  __currentNodeId = id" << std::endl <<
        "  setfenv(1, _Gcurrent)" << std::endl <<
        "end" << std::endl <<
        "" << std::endl <<
        "function input(name, type, ...)" << std::endl <<
        "  return __input[name][1]" << std::endl <<
        "end" << std::endl <<
        "" << std::endl <<
        "function getNodeId(name)" << std::endl <<
        "  return __input[name][2]" << std::endl <<
        "end" << std::endl <<
        "function output(name, type, val)" << std::endl <<
        "  setfenv(1, _G0)" << std::endl <<
        "  if (isDirty({ __currentNodeId })) then" << std::endl <<
        "    _G[name..__currentNodeId] = val" << std::endl <<
        "  end" << std::endl <<
        "  setfenv(1, _Gcurrent)" << std::endl <<
        "end" << std::endl <<
        "" << std::endl <<
        "function setDirty(node)" << std::endl <<
        "  __dirty[node] = true" << std::endl <<
        "end" << std::endl <<
        "" << std::endl <<
        "function isDirty(nodes)" << std::endl <<
        "  if first_exec then" << std::endl <<
        "    return true" << std::endl <<
        "  end" << std::endl <<
        "    " << std::endl <<
        "  if #nodes == 0 then" << std::endl <<
        "    return false" << std::endl <<
        "  else" << std::endl <<
        "    local node = table.remove(nodes, 1)" << std::endl <<
        "    if node == NIL then node = nil end" << std::endl <<
        "    return __dirty[node] or isDirty(nodes)" << std::endl <<
        "  end" << std::endl <<
        "end" << std::endl <<
        "" << std::endl <<
        "if first_exec == nil then" << std::endl <<
        "  first_exec = true" << std::endl <<
        "else" << std::endl <<
        "  first_exec = false" << std::endl <<
        "end" << std::endl <<
        "" << std::endl <<
        "emit(Void)" << std::endl <<
        "------------------------------------------------------" << std::endl;
      getMainGraph()->iceSL(file);
      file.close();
    }
  }

  //-------------------------------------------------------
  void NodeEditor::saveSettings()
  {
    // save current settings
    std::string filename(ChillFolder());
    filename += g_settingsFileName;

    // remove space from icesl path for storage in txt file
    std::string cleanedIceslPath = g_iceslPath;
    std::replace(cleanedIceslPath.begin(), cleanedIceslPath.end(), ' ', '#');

    std::ofstream f(filename);
    f << "icesl_path " << cleanedIceslPath << std::endl;
    f << "auto_save " << g_auto_save << std::endl;
    f << "auto_export " << g_auto_export << std::endl;
    f << "auto_launch_icesl " << g_auto_icesl << std::endl;
    f.close();
  }

  //-------------------------------------------------------
  void NodeEditor::loadSettings()
  {
    std::string filename = ChillFolder() + g_settingsFileName;
    if (LibSL::System::File::exists(filename.c_str())) {
      std::ifstream f(filename);
      while (!f.eof()) {
        std::string setting, value, raw;
        f >> setting >> value;
        if (setting == "icesl_path") {
          std::replace(value.begin(), value.end(), '#', ' ');
          g_iceslPath = value;
        }
        if (setting == "auto_save") {
          g_auto_save = (std::stoi(value) ? true : false);
        }
        if (setting == "auto_export") {
          g_auto_export = (std::stoi(value) ? true : false);
        }
        if (setting == "auto_launch_icesl") {
          g_auto_icesl = (std::stoi(value) ? true : false);
        }
      }
      f.close();
    }
  }

  //-------------------------------------------------------
  void NodeEditor::getScreenRes(int& width, int& height) {
#ifdef WIN32
    RECT screen;

    const HWND hScreen = GetDesktopWindow(); // get desktop handler
    GetWindowRect(hScreen, &screen); // get entire desktop size

    width = screen.right - screen.top;
    height = screen.bottom - screen.top;

#endif

#ifdef __linux__
    width = 1920;
    height = 1080;
#endif
  }

  //-------------------------------------------------------
  void NodeEditor::getDesktopScreenRes(int& width, int& height) {
#ifdef WIN32
    RECT desktop;

    // get desktop size WITHOUT task bar
    SystemParametersInfoA(SPI_GETWORKAREA, NULL, &desktop, NULL);

    width = desktop.right;
    height = desktop.bottom;

#endif

#ifdef __linux__
    width = 1920;
    height = 1080;
#endif
  }

  //-------------------------------------------------------
  void NodeEditor::launch()
  {
    NodeEditor *nodeEditor = NodeEditor::Instance();

    s_instance->loadSettings();
    s_instance->SetIceslPath();

    try {
      // create window
      SimpleUI::init(s_instance->default_width, s_instance->default_height, "Chill, the node-based editor");

      // attach functions
      SimpleUI::onRender = NodeEditor::mainRender;
      SimpleUI::onKeyPressed = NodeEditor::mainKeyPressed;
      SimpleUI::onScanCodePressed = NodeEditor::mainScanCodePressed;
      SimpleUI::onScanCodeUnpressed = NodeEditor::mainScanCodeUnpressed;
      SimpleUI::onMouseButtonPressed = NodeEditor::mainMousePressed;
      SimpleUI::onMouseMotion = NodeEditor::mainMouseMoved;
      SimpleUI::onReshape = NodeEditor::mainOnResize;

      // imgui
      SimpleUI::bindImGui();
      SimpleUI::initImGui();
      SimpleUI::onReshape(s_instance->default_width, s_instance->default_height);

      if (s_instance->g_auto_icesl) {
#ifdef WIN32
        SimpleUI::setCustomCallbackMsgProc(custom_wndProc);

        // get app handler
        s_instance->g_chill_hwnd = SimpleUI::getHWND();
#endif
        // launching Icesl
        launchIcesl();

        // place apps in default pos
        s_instance->setDefaultAppsPos();
      }

      // main loop
      SimpleUI::loop();

      if (s_instance->g_auto_icesl) {
        // closing Icesl
        std::atexit(closeIcesl);
      }

      s_instance->saveSettings();

      // clean up
      SimpleUI::terminateImGui();

      // shutdown UI
      SimpleUI::shutdown();

    }
    catch (Fatal& e) {
      std::cerr << Console::red << e.message() << Console::gray << std::endl;
    }
  }

  //-------------------------------------------------------
  void NodeEditor::setDefaultAppsPos() {
    int screen_width, screen_height, desktop_width, desktop_height = 0;

    // get screen / desktop dimmensions
    getScreenRes(screen_width, screen_height);
    getDesktopScreenRes(desktop_width, desktop_height);

    int app_width = 2 * (desktop_width / 3);
    int app_heigth = desktop_height;

    //int app_pos_x = desktop_width - screen_width;
    //int app_pos_y = desktop_height - screen_height;

    int app_pos_x = -8; // TODO  PB:get a correct offset / resolution calculation
    int app_pos_y = 0;

    // move to position
#ifdef WIN32
    SetWindowPos(g_chill_hwnd, HWND_TOPMOST, app_pos_x, app_pos_y, app_width, app_heigth, SWP_SHOWWINDOW);

    // move icesl to the last part of the screen
    int icesl_x_offset = 22; // TODO PB:get a correct offset / resolution calculation
    SetWindowPos(g_icesl_hwnd, HWND_TOP, app_width - icesl_x_offset, app_pos_y, screen_width - app_width + icesl_x_offset * 1.2, app_heigth, SWP_SHOWWINDOW);
#endif
  }

  //-------------------------------------------------------
  void NodeEditor::moveIceSLWindowAlongChill() {
#ifdef WIN32
    // get Chill current size
    RECT chillRect;
    GetWindowRect(g_chill_hwnd, &chillRect);
    int icesl_x_offset = 15; // TODO PB:get a correct offset / resolution calculation
    // move and resize according to Chill
    SetWindowPos(g_icesl_hwnd, HWND_NOTOPMOST, chillRect.right - icesl_x_offset, chillRect.top, ((chillRect.right - chillRect.left + 1) / 2) + icesl_x_offset * 2, chillRect.bottom - chillRect.top + 1, SWP_NOACTIVATE);
#endif
  }

  //-------------------------------------------------------
  std::string NodeEditor::ChillFolder() {
#ifdef WIN32
    return getenv("AppData") + std::string("\\Chill\\");
#elif __linux__
    return getenv("HOME") + std::string("/Chill/");
#endif
  }

  //-------------------------------------------------------
  std::string NodeEditor::NodesFolder() {
#ifdef WIN32
    return getenv("AppData") + std::string("\\Chill\\chill-nodes");
#elif __linux__
    return getenv("HOME") + std::string("/Chill/chill-nodes");
#endif
  }

  //-------------------------------------------------------
  void NodeEditor::SetIceslPath() {
    std::string defPath;
    if (g_iceslPath.empty()) {
#ifdef WIN32
      defPath = getenv("PROGRAMFILES");
      g_iceslPath = "C:\\Program Files\\INRIA\\IceSL\\bin\\IceSL-slicer.exe";
#elif __linux__
      std::string defPath = getenv("HOME");
      g_iceslPath = getenv("HOME") + std::string("/icesl/bin/IceSL-slicer");
#endif
    }
    if (!LibSL::System::File::exists(g_iceslPath.c_str())) {
      std::cerr << Console::red << "icesl not found" << Console::gray << std::endl;
      g_iceslPath = openFileDialog(OFD_FILTER_NONE).c_str();
      std::cerr << Console::yellow << "g_iceslPath: " << g_iceslPath << Console::gray << std::endl;
    }
  }

}
