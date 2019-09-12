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

//LIBSL_WIN32_FIX

namespace Chill
{
  NodeEditor* NodeEditor::s_instance = nullptr;

  //-------------------------------------------------------
  void listFolderinDir(std::vector<std::string>& _files, const std::string& _dir)
  {
    for (fs::directory_iterator itr(_dir); itr != fs::directory_iterator(); ++itr)
    {
      fs::path file = itr->path();
      if (is_directory(file))_files.push_back(file.filename().generic_string());
    }
  }

  void listLuaFileInDir(std::vector<std::string>& _files)
  {
    listFiles(NodeEditor::NodesFolder().c_str(), _files);
  }

  void listLuaFileInDir(std::vector<std::string>& _files, const std::string& _dir)
  {
    for (fs::directory_iterator itr(_dir); itr != fs::directory_iterator(); ++itr)
    {
      fs::path file = itr->path();
      if (!is_directory(file) && strcmp(extractExtension(file.filename().generic_string()).c_str(), "lua") == 0) {
        _files.push_back(file.filename().generic_string());
      }
    }
  }

  //---------------------------------------------------
  std::string recursiveFileSelecter(const std::string& _current_dir)
  {
    std::vector<std::string> files;
    listLuaFileInDir(files, _current_dir);
    std::vector<std::string> directories;
    std::string nameDir = "";
    listFolderinDir(directories, _current_dir);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ForIndex(i, directories.size()) {
      const char* dir_name = directories[i].c_str();
      if (dir_name[0] != '.' && ImGui::BeginMenu(dir_name)) {
        nameDir = recursiveFileSelecter(Resources::toPath(_current_dir, directories[i]));
        ImGui::EndMenu();
      }
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
    ForIndex(i, files.size()) {
      if (ImGui::MenuItem(removeExtensionFromFileName(files[i]).c_str())) {
        nameDir = Resources::toPath(_current_dir, files[i]);
      }
    }
    ImGui::PopStyleColor();

    return nameDir;
  }

  //-------------------------------------------------------
  std::string relativePath(const std::string& _path)
  {
    int nfsize = static_cast<int>(NodeEditor::NodesFolder().size());
    if (_path[nfsize + 1] == '/') nfsize++;
    std::string name = _path.substr(nfsize);
    return name;
  }

  //-------------------------------------------------------
  void addNodeMenu(ImVec2 _pos) {
    NodeEditor* n_e = NodeEditor::Instance();
    std::string node = recursiveFileSelecter(NodeEditor::NodesFolder());
    if (!node.empty()) {
      AutoPtr<LuaProcessor> proc = n_e->getCurrentGraph()->addProcessor<LuaProcessor>(relativePath(node));
      proc->setPosition(_pos);
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
    _In_ HWND   /*_hwnd*/,
    _In_ UINT   _uMsg,
    _In_ WPARAM /*_wParam*/,
    _In_ LPARAM /*_lParam*/)
  {
    // get current monitor hwnd
    HMONITOR hMonitor = MonitorFromWindow(NodeEditor::Instance()->g_chill_hwnd, MONITOR_DEFAULTTOPRIMARY);

    if (_uMsg == WM_MOVE) {
      NodeEditor::Instance()->moveIceSLWindowAlongChill();

      // get window placement / style
      WINDOWPLACEMENT wPlacement = { sizeof(WINDOWPLACEMENT) };
      GetWindowPlacement(NodeEditor::Instance()->g_chill_hwnd, &wPlacement);
      
      if (wPlacement.showCmd == SW_MAXIMIZE) {
        // set default pos
        NodeEditor::Instance()->setDefaultAppsPos(hMonitor);
      }
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

    setIcon();
  }

  //-------------------------------------------------------
  void NodeEditor::setIcon()
  {
    static bool icon_changed = false;
    if (!icon_changed) {
#ifdef WIN32
      HWND hWND = SimpleUI::getHWND();
      HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(16001));
      SendMessage(hWND, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
      SendMessage(hWND, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
#endif
      icon_changed = true;
    }
    // TODO: linux icon load
  }

  //-------------------------------------------------------
  void NodeEditor::mainOnResize(uint width, uint height)
  {
    Instance()->m_size = ImVec2(static_cast<float>(width), static_cast<float>(height));
    Instance()->moveIceSLWindowAlongChill();
  }

  //-------------------------------------------------------
  void NodeEditor::mainKeyPressed(uchar /*_k*/)
  {
  }

  //-------------------------------------------------------
  void NodeEditor::mainScanCodePressed(uint _sc)
  {
    if (_sc == LIBSL_KEY_SHIFT) {
      ImGui::GetIO().KeyShift = true;
    }
    if (_sc == LIBSL_KEY_CTRL) {
      ImGui::GetIO().KeyCtrl = true;
    }
  }

  //-------------------------------------------------------
  void NodeEditor::mainScanCodeUnpressed(uint _sc)
  {
    if (_sc == LIBSL_KEY_SHIFT) {
      ImGui::GetIO().KeyShift = false;
    }
    if (_sc == LIBSL_KEY_CTRL) {
      ImGui::GetIO().KeyCtrl = false;
    }
  }

  //-------------------------------------------------------
  void NodeEditor::mainMouseMoved(uint /*_x*/, uint /*_y*/)
  {
    // Nothing for now
  }

  //-------------------------------------------------------
  void NodeEditor::mainMousePressed(uint /*_x*/, uint /*_y*/, uint /*_button*/, uint /*_flags*/)
  {
    // Nothing for now
  }

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
  void NodeEditor::drawLeftMenu()
  {
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
      ImVec2 size(100, 20);

#ifdef WIN32
      uint i = 1;
      //TODO _Get_container is not standard
      for (auto graph : m_graphs._Get_container()) {
        std::string text = "";
        for (uint j = 0; j < i; ++j)
          text += " ";
        text += ">";
        ImGui::TextDisabled(text.c_str());
        ImGui::SameLine();

        if (i == m_graphs.size()) {
          strncpy_s(title, m_graphs.top()->name().c_str(), 32);
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
        ++i;
      }
#else
      // TODO
#endif

    }

    ImGui::NewLine();

    for ( auto object : selected) {
      ImGui::Text("Name:");
      strncpy(name, object->name().c_str(), 32);
      if (ImGui::InputText(("##name" + std::to_string(object->getUniqueID())).c_str(), name, 32)) {
        object->setName(name);
      }

      ImGui::Text("Color:");
      ImGuiColorEditFlags flags = ImGuiColorEditFlags_RGB | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaBar;
      ImVec4 col = ImGui::ColorConvertU32ToFloat4(object->color());
      float color[4] = { col.x, col.y , col.z , col.w };
      if (ImGui::ColorPicker4(("##color" + std::to_string(object->getUniqueID())).c_str(), color, flags)) {
        object->setColor(ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3])));
      }
      
      /*
      AutoPtr<Processor> proc = AutoPtr<Processor>(object);
      if (!proc.isNull()) {
        for (auto input : proc->inputs()) {
          ImGui::NewLine();
          if (input->drawTweak()) {
            proc->setDirty();
          }
        }
      }
      */

      ImGui::NewLine();
      
    }

    ImGui::End();
  }

  //-------------------------------------------------------
  void NodeEditor::drawGraph()
  {
    linking = !m_selected_input.isNull() || !m_selected_output.isNull();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, style.graph_bg_color);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
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

    zoom();

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO      io     = ImGui::GetIO();

    ImVec2 w_pos  = window->Pos;
    ImVec2 w_size = window->Size;
    float w_scale = window->FontWindowScale;

    ImVec2 offset = (m_offset * w_scale + (w_size - w_pos) / 2.0F);

    

    for (AutoPtr<Processor> processor : m_graphs.top()->processors()) {
      if (processor->isDirty()) {
        if (g_auto_export) {
          exportIceSL(g_iceSLTempExportPath);
//          exportIceSL(g_iceSLExportPath);
        }

        if (g_auto_save) {
          std::ofstream file;
          file.open(g_graphPath);
          m_graphs.top()->save(file);
        }
        break;
      }
    }

    if (ImGui::IsWindowHovered()) {
      // clear data
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

      // LEFT CLICK
      if (linking) {
        m_selecting = false;
        m_dragging = false;

        if (io.MouseDown[0] && (hovered.empty() || io.MouseDown[1])) {
          linking = false;
          m_selected_input = AutoPtr<ProcessorInput>(nullptr);
          m_selected_output = AutoPtr<ProcessorOutput>(nullptr);
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

          if (hovered.empty() && (!m_dragging || m_selecting)) {
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

      { // MOUSE WHEEL
        if (io.MouseWheel > 0 && w_scale == 2.0F && !hovered.empty()) {
          for (AutoPtr<SelectableUI> object : hovered) {
            AutoPtr<ProcessingGraph> pg(object);
            if (!pg.isNull()) {
              m_graphs.push(pg.raw());
              w_scale = 0.1F;
              window->FontWindowScale = 0.1F;
              break;
            }
          }
        }
      } // ! MOUSE WHEEL

      // Move selected processors
      if (m_dragging) {
        if (!io.KeysDown[LIBSL_KEY_CTRL]) {
          for (AutoPtr<SelectableUI> object : selected) {
            object->translate(io.MouseDelta / w_scale);
          }
        }
        else {
          for (AutoPtr<SelectableUI> object : selected) {
            AutoPtr<VisualComment> com(object);
            if (!com.isNull())
              object->m_size += io.MouseDelta / w_scale;
          }
        }
      }

      // Move the whole canvas
      if (io.MouseDown[2] && ImGui::IsMouseHoveringAnyWindow()) {
        m_offset += io.MouseDelta / w_scale;
      }

      // Keyboard
      if (!text_editing)
      {
        /*
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
        */
      }
      
      if (!selected.empty()) {
        if (io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['c' - 96] && io.KeysDownDuration['c' - 96] == 0.F) {
          buffer = getCurrentGraph()->copySubset(selected);
        }
      }


      if (!buffer.isNull()) {
        if (io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['v' - 96] && io.KeysDownDuration['v' - 96] == 0.F) {
          // mouse to screen
          ImVec2 m2s = io.MousePos - (w_pos + w_size) / 2.0F;
          // screen to grid
          ImVec2 s2g = m2s / w_scale - m_offset;

          AutoPtr<SelectableUI> copy_buff = buffer->clone();
          getCurrentGraph()->expandGraph(buffer, s2g);
          buffer = AutoPtr<ProcessingGraph>(copy_buff);
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
    float pipe_width = style.pipe_line_width * w_scale;
    int pipe_res = static_cast<int>(20 * w_scale);
    for (AutoPtr<Processor> processor : currentGraph->processors()) {
      for (AutoPtr<ProcessorOutput> output : processor->outputs()) {
        for (AutoPtr<ProcessorInput> input : output->m_links) {
          ImVec2 A = input->getPosition()  + w_pos - ImVec2(pipe_width / 4.F, 0.F);
          ImVec2 B = output->getPosition() + w_pos + ImVec2(pipe_width / 4.F, 0.F);

          ImVec2 bezier(100 * w_scale, 0.0F);

          ImGui::GetWindowDrawList()->AddBezierCurve(
            A,
            A - bezier,
            B + bezier,
            B,
            //processor->color(),
            input->color(),
            pipe_width,
            pipe_res
          );

          ImVec2 center = (A + B) / 2.F;
          ImVec2 vec = (A*2.F - bezier) - (B*2.F + bezier);
          ImVec2 norm_vec = (vec / sqrt(vec.x*vec.x + vec.y*vec.y)) * 2.0F*pipe_width;


          ImVec2 U = center + norm_vec * 2.F;
          std::swap(norm_vec.x, norm_vec.y);
          ImVec2 V = center + norm_vec;
          ImVec2 W = center - norm_vec;
          std::swap(V.x, W.x);

          ImGui::GetWindowDrawList()->AddTriangleFilled(U, V, W, input->color());
        }
      }
    }

    // Draw new pipe
    if (linking) {
      ImVec2 A = ImGui::GetMousePos();
      ImVec2 B = ImGui::GetMousePos();

      if (!m_selected_input.isNull()) {
        A = w_pos + m_selected_input->getPosition();
        A -= ImVec2(pipe_width / 4.F, 0.F);
      }
      else if (!m_selected_output.isNull()) {
        B = w_pos + m_selected_output->getPosition();
        B += ImVec2(pipe_width / 4.F, 0.F);
      }

      ImVec2 bezier(100.0F * w_scale, 0.0F);
      ImGui::GetWindowDrawList()->AddBezierCurve(
        A,
        A - bezier,
        B + bezier,
        B,
        style.pipe_selected_color,
        pipe_width,
        pipe_res
      );

      ImGui::GetWindowDrawList()->AddCircleFilled(A, pipe_width / 2.0F, style.pipe_selected_color);
      ImGui::GetWindowDrawList()->AddCircleFilled(B, pipe_width / 2.0F, style.pipe_selected_color);
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

    if (io.FontAllowUserScaling) {
      float old_scale = window->FontWindowScale;
      float new_scale = ImClamp(
        window->FontWindowScale + io.MouseWheel * exp(0.1F/window->FontWindowScale)/5.0F,
        0.1F,
        2.0F);


      // ToDo : Move this elsewhere !!
      if (old_scale <= 0.1F && io.MouseWheel < 0 && m_graphs.size() > 1) {
        m_graphs.pop();
        new_scale = 2.0F;
      }

      window->FontWindowScale = new_scale;

      if (io.MouseWheel < 0 || 0 < io.MouseWheel) {
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

    ImVec2 offset = (m_offset * window->FontWindowScale + (window->Size - window->Pos) / 2.F);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const ImU32& grid_Color      = style.graph_grid_color;
    const float& grid_Line_width = style.graph_grid_line_width;

    const int subdiv = window->FontWindowScale >= 1.0F ? 10 : 100;

    int grid_size  = static_cast<int>(window->FontWindowScale * subdiv);
    int offset_x   = static_cast<int>(offset.x);
    int offset_y   = static_cast<int>(offset.y);

    int i_x = offset_x / grid_size;
    int i_y = offset_y / grid_size;

    // Vertical lines
    for (int x = offset_x % grid_size; x < window->Size.x; x += grid_size) {
      ImVec2 x1 = window->Pos + ImVec2(x, 0             );
      ImVec2 x2 = window->Pos + ImVec2(x, window->Size.y);
      draw_list->AddLine(x1, x2, grid_Color, grid_Line_width);

      if (i_x % 10 == 0) draw_list->AddLine(x1, x2, grid_Color, grid_Line_width);
      --i_x;
    }

    // Horizontal lines
    for (int y = offset_y % grid_size; y < window->Size.y; y += grid_size) {
      ImVec2 y1 = window->Pos + ImVec2(0             , y);
      ImVec2 y2 = window->Pos + ImVec2(window->Size.x, y);
      draw_list->AddLine(y1, y2, grid_Color, grid_Line_width);

      if (i_y % 10 == 0) draw_list->AddLine(y1, y2, grid_Color, grid_Line_width);
      --i_y;
    }
  }

  //-------------------------------------------------------
  void NodeEditor::menus() {
    NodeEditor* n_e = Instance();

    ImGuiWindow* window = ImGui::GetCurrentWindow();


    ImGuiIO io = ImGui::GetIO();

    // Draw menus
    if (Instance()->m_graph_menu) {
      ImGui::OpenPopup("graph_menu");
    }
    if (Instance()->m_node_menu) {
      ImGui::OpenPopup("node_menu");
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
    if (ImGui::BeginPopup("graph_menu"))
    {

      ImVec2 w_pos = window->Pos;
      ImVec2 w_size = window->Size;
      float w_scale = window->FontWindowScale;

      // mouse to screen
      ImVec2 m2s = io.MousePos - (w_pos + w_size) / 2.0F;
      // screen to grid
      ImVec2 s2g = m2s / w_scale - Instance()->m_offset;

      Instance()->m_graph_menu = false;

      /*
      if (ImGui::BeginMenu("Add", "SPACE")) {
      */
        addNodeMenu(s2g);
      /*
        ImGui::EndMenu();
      }
      */

      if (!buffer.isNull()) {
        if (ImGui::MenuItem("Paste", "CTRL+V")) {
          // mouse to screen
          ImVec2 m2s = io.MousePos - (w_pos + w_size) / 2.0F;
          // screen to grid
          ImVec2 s2g = m2s / w_scale - m_offset;

          AutoPtr<SelectableUI> copy_buff = buffer->clone();
          getCurrentGraph()->expandGraph(buffer, s2g);
          buffer = AutoPtr<ProcessingGraph>(copy_buff);
        }
      }
      /*
      if (ImGui::MenuItem("Add multiplexer node")) {
        AutoPtr<Multiplexer> proc = n_e->getCurrentGraph()->addProcessor<Multiplexer>();
        proc->setPosition(s2g);
      }
      

      if (ImGui::MenuItem("Comment")) {
        AutoPtr<VisualComment> com(new VisualComment());
        com->setPosition(s2g);
        n_e->getCurrentGraph()->addComment(com);
      }
      */

      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("node_menu"))
    {
      Instance()->m_node_menu = false;
      if (ImGui::MenuItem("Copy", "CTRL+C"))
      {
        buffer = n_e->getCurrentGraph()->copySubset(selected);
      }
      /*
      if (ImGui::MenuItem("Group"))
      {
        n_e->getCurrentGraph()->collapseSubset(selected);
      }
      if (ImGui::MenuItem("Ungroup")) {
        for (AutoPtr<SelectableUI> proc : selected) {
          if (typeid(*proc.raw()) == typeid(ProcessingGraph)) {
            n_e->getCurrentGraph()->expandGraph(AutoPtr<ProcessingGraph>(proc), proc->getPosition());
          }
        }
      }
      */
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
              Processor::disconnect(input);
            }
            for (AutoPtr<ProcessorOutput> output : proc->outputs()) {
              Processor::disconnect(output);
            }
          }
        }
      }
      ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
  }

  //-------------------------------------------------------
  void NodeEditor::selectProcessors() {
    Instance()->m_selecting = true;

    NodeEditor* n_e = Instance();
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2  w_pos = window->Pos;
    ImVec2 w_size = window->Size;
    float w_scale = window->FontWindowScale;

    ImGuiIO io = ImGui::GetIO();

    ImVec2 A = (io.MousePos - (w_pos + w_size) / 2.0F) / w_scale - Instance()->m_offset;
    ImVec2 B = (io.MouseClickedPos[0] - (w_pos + w_size) / 2.0F) / w_scale - Instance()->m_offset;

    if (A.x > B.x) std::swap(A.x, B.x);
    if (A.y > B.y) std::swap(A.y, B.y);

    if (B.x - A.x > 10 || B.y - A.y > 10) {
      draw_list->AddRect(
        io.MouseClickedPos[0],
        io.MousePos,
        Instance()->style.processor_selected_color
      );

      if (!io.KeysDown[LIBSL_KEY_SHIFT]) {
        for (AutoPtr<SelectableUI> selproc : selected) {
          selproc->m_selected = false;
        }
        selected.clear();
      }

      auto isInside = [](ImVec2 min, ImVec2 max, ImVec2& A, ImVec2& B) {
        return !(min.x < A.x || B.x < max.x || min.y < A.y || B.y < max.y);
      };

      for (AutoPtr<Processor> procui : n_e->getCurrentGraph()->processors()) {
        ImVec2 pos_min = procui->getPosition();
        ImVec2 pos_max = pos_min + procui->m_size;
        procui->m_selected = isInside(pos_min, pos_max, A, B);
        selected.push_back(AutoPtr<SelectableUI>(procui));
      }

      for (AutoPtr<VisualComment> comui : n_e->getCurrentGraph()->comments()) {
        ImVec2 pos_min = comui->getPosition();
        ImVec2 pos_max = pos_min + comui->m_size;
        comui->m_selected = isInside(pos_min, pos_max, A, B);
        selected.push_back(AutoPtr<SelectableUI>(comui));
      }
    }
  }

  //-------------------------------------------------------
  void NodeEditor::launchIcesl() {
#ifdef WIN32
    // CreateProcess init
    const char* icesl_path = Instance()->g_iceslPath.c_str();
    std::string icesl_params = " " + Instance()->g_iceSLTempExportPath.string();

    STARTUPINFO StartupInfo;
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof StartupInfo;

    ZeroMemory(&Instance()->g_icesl_p_info, sizeof(Instance()->g_icesl_p_info));

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
      &Instance()->g_icesl_p_info); // @lpProcessInformation - process info

    if (icesl_process)
    {
      // watch the process
      WaitForSingleObject(Instance()->g_icesl_p_info.hProcess, 1000);
      // getting the hwnd
      EnumWindows(EnumWindowsFromPid, Instance()->g_icesl_p_info.dwProcessId);
      if (Instance()->g_auto_export) {
        Instance()->exportIceSL(Instance()->g_iceSLTempExportPath);
      }
    }
    else
    {
      // process creation failed
      std::cerr << Console::red << "Icesl couldn't be opened, please launch Icesl manually" << Console::gray << std::endl;
      std::cerr << Console::red << "ErrorCode: " << GetLastError() << Console::gray << std::endl;

      Instance()->g_icesl_hwnd = NULL;
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
  void NodeEditor::exportIceSL(const fs::path filename) {
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
        "function setColor(...) end" << std::endl <<
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
    std::string filename(ChillFolder() + g_settingsFileName);
    //filename += g_settingsFileName;

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
  void NodeEditor::getDesktopRes(int& width, int& height) {
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

    NodeEditor *nodeEditor = Instance();

    nodeEditor->loadSettings();
    nodeEditor->SetIceslPath();

    // create the temp file
    nodeEditor->exportIceSL(Instance()->g_iceSLTempExportPath.string());

    try {
      // create window
      SimpleUI::init(nodeEditor->default_width, nodeEditor->default_height, "Chill, the node-based editor");

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
      SimpleUI::onReshape(nodeEditor->default_width, nodeEditor->default_height);

      if (nodeEditor->g_auto_icesl) {
#ifdef WIN32
        SimpleUI::setCustomCallbackMsgProc(custom_wndProc);

        // get app handler
        Instance()->g_chill_hwnd = SimpleUI::getHWND();
#endif
        // launching Icesl
        launchIcesl();

        // place apps in default pos
#ifdef WIN32
        Instance()->setDefaultAppsPos(NULL);
#else
        nodeEditor->setDefaultAppsPos();
#endif
      }

      // main loop
      SimpleUI::loop();

      if (nodeEditor->g_auto_icesl) {
        // closing Icesl
        std::atexit(closeIcesl);
      }

      nodeEditor->saveSettings();

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
#ifdef WIN32
  void NodeEditor::setDefaultAppsPos(HMONITOR hMonitor) {
    int desktop_width, desktop_height = 0;

    // get desktop dimmensions
    getDesktopRes(desktop_width, desktop_height);

    // set chill dimensions to be 2/3 - 1/3 with icesl
    int app_width = 2 * (desktop_width / 3);
    int app_heigth = desktop_height;
    int app_x_offset = 8; // TODO  PB:get a correct offset / resolution calculation

    // get current monitor infos
    MONITORINFO monitorInfo = {sizeof(MONITORINFO) };
    GetMonitorInfo(hMonitor, &monitorInfo);;

    // get center point of current monitor
    POINT monitor_center;
    monitor_center.x = monitorInfo.rcMonitor.right / 2;
    monitor_center.y = monitorInfo.rcMonitor.bottom / 2;

    //get hwnd for desktop on current monitor
    HWND hDesktop = WindowFromPoint(monitor_center);

    // move chill to position
    SetWindowPos(g_chill_hwnd, hDesktop, monitorInfo.rcMonitor.left - app_x_offset, monitorInfo.rcMonitor.top, app_width, app_heigth, SWP_SHOWWINDOW);

    // move icesl to the last part of the screen
    int icesl_x_offset = 22; // TODO PB:get a correct offset / resolution calculation
    int icesl_xpos = app_width - icesl_x_offset;
    int icesl_ypos = monitorInfo.rcMonitor.top;
    int icesl_width = desktop_width - app_width + icesl_x_offset * 1.2;

    SetWindowPos(g_icesl_hwnd, hDesktop, icesl_xpos, icesl_ypos, icesl_width, app_heigth, SWP_SHOWWINDOW | SWP_NOACTIVATE);
  }

#else
  void NodeEditor::setDefaultAppsPos() {

    // TODO linux move chill window

    // TODO linux move icesl window
  }
#endif

  //-------------------------------------------------------
  void NodeEditor::moveIceSLWindowAlongChill() {
#ifdef WIN32
    // get Chill current size
    RECT chillRect;
    GetWindowRect(g_chill_hwnd, &chillRect);
    int icesl_x_offset = 15; // TODO PB:get a correct offset / resolution calculation
    // move and resize according to Chill

    HMONITOR hMonitor = MonitorFromWindow(g_chill_hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
    GetMonitorInfo(hMonitor, &monitorInfo);

    SetWindowPos(g_icesl_hwnd, HWND_NOTOPMOST, chillRect.right - icesl_x_offset, chillRect.top, ((chillRect.right - chillRect.left + 1) / 2) + icesl_x_offset * 2, chillRect.bottom - chillRect.top, SWP_NOACTIVATE);
#endif
  }

  //-------------------------------------------------------
  bool NodeEditor::scriptPath(const std::string& name, std::string& _path)
  {
    std::string         path;
    std::vector<std::string> paths;

#ifdef WIN32
    paths.push_back(getenv("APPDATA") + std::string("/Chill") + name);
#endif
    paths.push_back(fs::current_path().string() + name);
    paths.push_back(".." + name);
    paths.push_back("../.." + name);
    paths.push_back("../../.." + name);
    paths.push_back("../../../.." + name);
    paths.push_back("../../../../Source/Chill" + name); // dev path
    paths.push_back("../../../Source/Chill" + name); // dev path
    
    bool ok = false;
    ForIndex(p, paths.size()) {
      path = paths[p];
      if (fs::exists(path)) {
        ok = true;
        break;
      }
    }
    if (!ok) {
      std::cerr << Console::red << "Cannot find path for " << name << Console::gray << std::endl;
      std::cerr << Console::yellow << "path? : " << path << Console::gray << std::endl;
      return false;
    }
    _path = path;
    return true;
  }
  //-------------------------------------------------------
  std::string NodeEditor::ChillFolder() {
    std::string chillFolder;
    std::string folderName = "";
    if (scriptPath(folderName, chillFolder)) {
      return chillFolder;
    }
    else {
      return fs::current_path().string();
    }
  }

  //-------------------------------------------------------
  std::string NodeEditor::NodesFolder() {
    std::string nodesFolder;
    std::string folderName = "/chill-nodes";

    if (scriptPath(folderName, nodesFolder)) {
      return nodesFolder;
    }
    else {
      return fs::current_path().string();
    }
  }

  //-------------------------------------------------------
  void NodeEditor::SetIceslPath() {
    if (g_iceslPath.empty()) {
#ifdef WIN32
      g_iceslPath = getenv("PROGRAMFILES") + std::string("/INRIA/IceSL/bin/IceSL-slicer.exe");
#elif __linux__
      g_iceslPath = getenv("HOME") + std::string("/icesl/bin/IceSL-slicer");
#endif
    }
    if (!LibSL::System::File::exists(g_iceslPath.c_str())) {
      std::cerr << Console::red << "IceSL executable not found. Please specify the location of IceSL's executable." << Console::gray << std::endl;

      const char * modalTitle = "IceSL was not found...";
      const char * modalText = "Icesl was not found on this computer.\n\nIn order for Chill to work properly, it needs access to IceSL.\n\nPlease specify the location of IceSL's executable on your computer.";

#ifdef WIN32
      uint modalFlags = MB_OKCANCEL | MB_DEFBUTTON1 | MB_SYSTEMMODAL | MB_ICONINFORMATION;

      int modal = MessageBox(g_chill_hwnd, modalText, modalTitle, modalFlags);

      if (modal == 1) {
        g_iceslPath = openFileDialog(OFD_FILTER_ALL).c_str();
        std::cerr << Console::yellow << "IceSL location specified: " << g_iceslPath << Console::gray << std::endl;
      }
      else {
        g_iceslPath = "";
      }

#elif __linux__
      // TODO Linux modal window
      g_iceslPath = openFileDialog(OFD_FILTER_ALL).c_str();
      std::cerr << Console::yellow << "IceSL location specified: " << g_iceslPath << Console::gray << std::endl;
#endif
    }
  }

}
