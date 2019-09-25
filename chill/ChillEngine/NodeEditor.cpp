#include "NodeEditor.h"
#include "Processor.h"
#include "LuaProcessor.h"
#include "IOs.h"
#include "GraphSaver.h"
#include "FileDialog.h"
#include "Resources.h"
#include "VisualComment.h"

#include "SourcePath.h"

#ifdef USE_GLUT
#include <GL/glut.h>
#endif

#undef ForIndex
#define ForIndex(I, N) for(decltype(N) I=0; I<N; I++)

//LIBSL_WIN32_FIX

namespace chill
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
  std::string recursiveFileMenuSelecter(const std::string& _current_dir, std::string filter = "")
  {
    std::vector<std::string> files;
    std::vector<std::string> directories;

    listLuaFileInDir(files, _current_dir);
    listFolderinDir(directories, _current_dir);

    std::string nameDir = "";

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ForIndex(i, directories.size()) {
      const char* dir_name = directories[static_cast<unsigned>(i)].c_str();
      if (!nameDir.empty()) break;

      if (dir_name[0] != '.') {
        if (!filter.empty()){
          nameDir = recursiveFileMenuSelecter(Resources::toPath(_current_dir, directories[i]), filter);
        } else {
          if (ImGui::CollapsingHeader((std::string(dir_name) + "##" + _current_dir).c_str() )) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
            ImGui::BeginGroup();
            nameDir = recursiveFileMenuSelecter(Resources::toPath(_current_dir, directories[i]), filter);
            ImGui::EndGroup();

          }
        }
      }
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
    ForIndex(i, files.size()) {
      std::string name = removeExtensionFromFileName(files[i]);
      std::string low_name = name;
      std::locale loc;

      for(auto& elem : low_name)
          elem = std::tolower(elem,loc);

      // If don't match the filter, we skip
      if (low_name.find(filter) == std::string::npos) {
        continue;
      }

      bool test = ImGui::MenuItem(name.c_str());

      if (test) {
        nameDir = Resources::toPath(_current_dir, files[i]);
      }

    }
    ImGui::PopStyleColor();
    return nameDir;
  }

  //---------------------------------------------------
  std::string recursiveFileSelecter(const std::string& _current_dir, std::string filter = "")
  {
    std::vector<std::string> files;
    std::vector<std::string> directories;

    listLuaFileInDir(files, _current_dir);
    listFolderinDir(directories, _current_dir);

    std::string nameDir = "";

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    ForIndex(i, directories.size()) {
      const char* dir_name = directories[static_cast<unsigned>(i)].c_str();
      if (!nameDir.empty()) break;

      if (dir_name[0] != '.') {
        if (!filter.empty()){
          nameDir = recursiveFileSelecter(Resources::toPath(_current_dir, directories[i]), filter);
        } else {
          if (ImGui::BeginMenu(dir_name)) {
            nameDir = recursiveFileSelecter(Resources::toPath(_current_dir, directories[i]), filter);
            ImGui::EndMenu();
          }
        }
      }
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
    ForIndex(i, files.size()) {
      std::string name = removeExtensionFromFileName(files[i]);
      std::string low_name = name;
      std::locale loc;

      for(auto& elem : low_name)
          elem = std::tolower(elem,loc);

      // If don't match the filter, we skip
      if (low_name.find(filter) == std::string::npos) {
        continue;
      }

      bool test = ImGui::MenuItem(name.c_str());

      if (test) {
        nameDir = Resources::toPath(_current_dir, files[i]);
      }

    }
    ImGui::PopStyleColor();
    return nameDir;
  }

  //-------------------------------------------------------
  std::string relativePath(const std::string& _path) {
    unsigned long nfsize = static_cast<unsigned long>(NodeEditor::NodesFolder().size());
    if (_path[nfsize + 1] == '/') nfsize++;
    std::string name = _path.substr(nfsize);
    return name;
  }

  //-------------------------------------------------------
  bool addNodeLeftMenu(ImVec2 _pos, std::string filter = "") {
    NodeEditor* n_e = NodeEditor::Instance();
    std::string node = recursiveFileMenuSelecter(NodeEditor::NodesFolder(), filter);
    if (!node.empty()) {
      std::shared_ptr<LuaProcessor> proc = n_e->getCurrentGraph()->addProcessor<LuaProcessor>(relativePath(node));
      proc->setPosition(_pos);
      return true;
    }
    return false;
  }

  //-------------------------------------------------------
  bool addNodeMenu(ImVec2 _pos, std::string filter = "") {
    NodeEditor* n_e = NodeEditor::Instance();
    std::string node = recursiveFileSelecter(NodeEditor::NodesFolder(), filter);
    if (!node.empty()) {
      std::shared_ptr<LuaProcessor> proc = n_e->getCurrentGraph()->addProcessor<LuaProcessor>(relativePath(node));
      proc->setPosition(_pos);
      return true;
    }
    return false;
  }

  //-------------------------------------------------------
#ifdef WIN32
  BOOL CALLBACK EnumWindowsFromPid(HWND hwnd, LPARAM lParam)
  {
    DWORD pID;
    GetWindowThreadProcessId(hwnd, &pID);
    if (pID == lParam)
    {
      NodeEditor::Instance()->m_icesl_hwnd = hwnd;
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
    _In_ WPARAM _wParam,
    _In_ LPARAM _lParam)
  {
    // get current monitor hwnd
    //HMONITOR hMonitor = MonitorFromWindow(NodeEditor::Instance()->m_chill_hwnd, MONITOR_DEFAULTTOPRIMARY);
    
    if (_uMsg == WM_ACTIVATE) {
      if (_wParam != WA_ACTIVE ) {
        //SetWindowPos(NodeEditor::Instance()->m_chill_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        //BringWindowToTop(NodeEditor::Instance()->m_icesl_hwnd);
        BringWindowToTop(NodeEditor::Instance()->m_chill_hwnd);
        //SetForegroundWindow(NodeEditor::Instance()->m_chill_hwnd);
        //SetWindowPos(NodeEditor::Instance()->m_chill_hwnd, NodeEditor::Instance()->m_icesl_hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
        //NodeEditor::Instance()->showIceSL();
      }
    }

    if (_uMsg == WM_MOVE) {
      NodeEditor::Instance()->moveIceSLWindowAlongChill(true,false);

      // get window placement / style
      WINDOWPLACEMENT wPlacement = { sizeof(WINDOWPLACEMENT) };
      GetWindowPlacement(NodeEditor::Instance()->m_chill_hwnd, &wPlacement);
      
      if (wPlacement.showCmd == SW_MAXIMIZE) {
        NodeEditor::Instance()->maximize();
      }
    }

    if (_uMsg == WM_SYSCOMMAND) {
      if (_wParam == SC_MINIMIZE) {
        NodeEditor::Instance()->m_minimized = true;
      }
      if (_wParam == SC_RESTORE) {
        NodeEditor::Instance()->m_minimized = false;
      }
    }
    return 0;
  }
#endif

  //-------------------------------------------------------

  NodeEditor::NodeEditor() {
    m_graphs.push(std::shared_ptr<ProcessingGraph>(new ProcessingGraph()));
  }

  //-------------------------------------------------------

  NodeEditor* NodeEditor::Instance() {
    if (!s_instance) {
      s_instance = new NodeEditor();

      std::string filename = Instance()->ChillFolder() + "/init.graph";
      if (fs::exists(filename.c_str())) {
        GraphSaver test;
        test.execute(filename.c_str());

      }
    }
    return s_instance;
  }

  //-------------------------------------------------------

  void NodeEditor::mainRender() {
    glClearColor(0.F, 0.F, 0.F, 0.F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Instance()->draw();

    ImGui::Render();

    setIcon();
  }

  //-------------------------------------------------------

  void NodeEditor::setIcon() {
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
    Instance()->moveIceSLWindowAlongChill(false,false);
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
    /*if (m_docking_icesl && m_icesl_hwnd) {
      SetWindowLongPtr(m_icesl_hwnd, GWL_STYLE, WS_VISIBLE | WS_CHILD);
    }*/
    drawMenuBar();
    ImGui::SetNextWindowPos(ImVec2(0, 20));
    ImGui::SetNextWindowSize(ImVec2(200, m_size.y - 20));
    drawLeftMenu();

    ImGui::SetNextWindowPos(ImVec2(200, 20));
    ImGui::SetNextWindowSize(m_size);
    drawGraph();

    //for docking
    updateIceSLPosRatio();
    showIceSL();

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
            setMainGraph(std::shared_ptr<ProcessingGraph>(new ProcessingGraph()));
            getMainGraph()->save(file);
            file.close();
            m_graphPath = fullpath;
          }
        }
        if (ImGui::MenuItem("Load graph")) {
          fullpath = openFileDialog(OFD_FILTER_GRAPHS);
          if (!fullpath.empty()) {
            GraphSaver test;
            test.execute(fullpath.c_str());
            m_graphPath = fullpath;
          }
        }
        if (ImGui::MenuItem("Load example graph")) {
          std::string chillFolder;
          // TODO clean this !
#ifdef WIN32
          std::string folderName = "\\chill-models\\";
#else
          std::string folderName = "/chill-models/";
#endif
          if (scriptPath(folderName, chillFolder)) {
            fullpath = openFileDialog(chillFolder.c_str(), OFD_FILTER_GRAPHS);
            if (!fullpath.empty()) {
              GraphSaver test;
              test.execute(fullpath.c_str());
              m_graphPath = fullpath;
            }
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
            m_graphPath = fullpath;
          }
        }
        /*
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
        */

        if (ImGui::MenuItem("Export to IceSL lua")) {
          std::string graph_filename = getMainGraph()->name() + ".lua";
          m_iceSLExportPath = saveFileDialog(graph_filename.c_str(), OFD_FILTER_LUA);
          exportIceSL(m_iceSLExportPath);
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Settings")) {
        ImGui::MenuItem("Automatic save", "", &m_auto_save);
        ImGui::MenuItem("Automatic export", "", &m_auto_export);
        ImGui::MenuItem("Automatic use of IceSL", "", &m_auto_icesl);
        ImGui::EndMenu();
      }

#ifdef WIN32
      if (ImGui::BeginMenu("Window")) {

        bool docking = m_icesl_is_docked;
        if (ImGui::MenuItem("Dock IceSL in view", "Ctrl + D", &docking)) {
          if (docking) { // just changed
            dock();
          } else {
            undock();
          }
        }

        if (m_icesl_is_docked) {
          const char *items[sizeof(layouts) / sizeof(layouts[0])];
          for (int i = 0; i < sizeof(layouts) / sizeof(layouts[0]); i++) {
            items[i] = layouts[i].Name;
          }
          static int item_current = 0;
          if (ImGui::Combo("##combo", &item_current, items, IM_ARRAYSIZE(items))) {
            setLayout(item_current);
          }
        }

        ImGui::EndMenu();
      }
#endif
      // save the effective menubar height in the style
      style.menubar_height = ImGui::GetWindowSize().y;
      ImGui::EndMainMenuBar();
    }

    ImGui::PopStyleColor();
  }

  //-------------------------------------------------------
  char leftMenuSearch[64] = "";
  void NodeEditor::drawLeftMenu()
  {
    ImGui::Begin("GraphInfo", &m_visible,
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse |

      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoResize |

      ImGuiWindowFlags_NoBringToFrontOnFocus
    );

#if 0
    char name[32];

    if (ImGui::CollapsingHeader("Graph tree")) {
      ImVec2 size(100, 20);

#ifdef WIN32
      char title[32];
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
      std::shared_ptr<Processor> proc = std::shared_ptr<Processor>(object);
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
#endif

    ImVec2 s2g = ImVec2(-100, -50) - Instance()->m_offset;

    ImGui::InputTextWithHint("##", "search", leftMenuSearch,  64);

    std::locale loc;
    for( auto& elem: leftMenuSearch)
       elem = std::tolower(elem, loc);
    addNodeLeftMenu(s2g, leftMenuSearch);


    ImGui::End();
  }

  //-------------------------------------------------------
  void NodeEditor::drawGraph()
  {
    linking = m_selected_input || m_selected_output;

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

    
    bool wasDirty = false;
    for (std::shared_ptr<Processor> processor : *m_graphs.top()->processors()) {
      if (processor->isDirty()) {
        wasDirty = true;
        processor->setDirty(false);
      }
    }

    if (wasDirty) {
      if (m_auto_export) {
        exportIceSL(m_iceSLTempExportPath);
      }

      if (m_auto_save) {
        std::ofstream file;
        file.open(m_graphPath);
        m_graphs.top()->save(file);
      }
    }

    if (ImGui::IsWindowHovered()) {
      // clear data
      hovered.clear();
      selected.clear();
      text_editing = false;

      for (std::shared_ptr<Processor> processor : *m_graphs.top()->processors()) {
        ImVec2 socket_size = ImVec2(1, 1) * (style.socket_radius + style.socket_border_width) * w_scale;

        ImVec2 size = processor->m_size * window->FontWindowScale;
        ImVec2 min_pos = offset + w_pos + processor->m_position * w_scale - socket_size;
        ImVec2 max_pos = min_pos + size + socket_size * 2;

        if (ImGui::IsMouseHoveringRect(min_pos, max_pos)) {
          hovered.push_back(std::shared_ptr<SelectableUI>(processor));
        }

        if (processor->m_selected) {
          selected.push_back(std::shared_ptr<SelectableUI>(processor));
        }

        if (processor->m_edit) {
          text_editing = true;
        }
      }

      for (std::shared_ptr<VisualComment> comment : m_graphs.top()->comments()) {
        ImVec2 socket_size = ImVec2(1, 1) * (style.socket_radius + style.socket_border_width) * w_scale;

        ImVec2 size = comment->m_title_size * w_scale;
        ImVec2 min_pos = offset + w_pos + comment->m_position * w_scale - socket_size;
        ImVec2 max_pos = min_pos + size + socket_size * 2;

        if (ImGui::IsMouseHoveringRect(min_pos, max_pos)) {
          hovered.push_back(std::shared_ptr<SelectableUI>(comment));
        }

        if (comment->m_selected) {
          selected.push_back(std::shared_ptr<SelectableUI>(comment));
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
          m_selected_input = std::shared_ptr<ProcessorInput>(nullptr);
          m_selected_output = std::shared_ptr<ProcessorOutput>(nullptr);
        }
      }
      else {
        if (io.MouseDoubleClicked[0]) {
          for (std::shared_ptr<SelectableUI> hovproc : hovered) {
            std::shared_ptr<ProcessingGraph> v = std::dynamic_pointer_cast<ProcessingGraph>(hovproc);
            if (v && !v->m_edit) {
              m_graphs.push(v);
              m_offset = m_graphs.top()->getBarycenter() * -1.0F;
              break;
            }
          }
        }

        if (io.MouseDown[0] && io.MouseDownOwned[0]) {
          if (!hovered.empty() && !m_selecting) {
            for (std::shared_ptr<SelectableUI> selproc : selected) {
              for (std::shared_ptr<SelectableUI> hovproc : hovered) {
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
            for (std::shared_ptr<SelectableUI> selproc : selected) {
              selproc->m_selected = false;
            }
            selected.clear();
          }
          else {


            for (std::shared_ptr<SelectableUI> hovproc : hovered) {
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
                for (std::shared_ptr<SelectableUI> selproc : selected) {
                  if (selproc == hovproc) {
                    sel_and_hov = true;
                    break;
                  }
                }
                if (!sel_and_hov) {
                  for (std::shared_ptr<SelectableUI> selproc : selected) {
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

#if 0
      { // MOUSE WHEEL to enter sub-graph
        if (io.MouseWheel > 0 && w_scale == 2.0F && !hovered.empty()) {
          for (std::shared_ptr<SelectableUI> object : hovered) {
            std::shared_ptr<ProcessingGraph> pg = std::static_pointer_cast<ProcessingGraph>(object);
            if (pg) {
              m_graphs.push(pg.get());
              w_scale = 0.1F;
              window->FontWindowScale = 0.1F;
              break;
            }
          }
        }
      } // ! MOUSE WHEEL
#endif
      // Move selected processors
      if (m_dragging) {
        if (!io.KeysDown[LIBSL_KEY_CTRL]) {
          for (std::shared_ptr<SelectableUI> object : selected) {
            object->translate(io.MouseDelta / w_scale);
          }
        }
        else {
          for (std::shared_ptr<SelectableUI> object : selected) {
            std::shared_ptr<VisualComment> com = std::static_pointer_cast<VisualComment>(object);
            if (com)
              object->m_size += io.MouseDelta / w_scale;
          }
        }
      }

      // Move the whole canvas
      if (io.MouseDown[2] && ImGui::IsWindowHovered()) {
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
          std::shared_ptr<VisualComment> com(new VisualComment());
          com->setPosition(s2g);
          this->getCurrentGraph()->addComment(com);
        }
        */
      }
      
      if (!selected.empty()) {
        if (io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['c' - 96] && io.KeysDownDuration['c' - 96] == 0.F) {
          copy();
        }
      }
      if (io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['d' - 96] && io.KeysDownDuration['d' - 96] == 0.F) {
        if (!m_icesl_is_docked) {
          dock();
        } else {
          undock();
        }
      }
      /*
      if (io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['1' - 96] && io.KeysDownDuration['1' - 96] == 0.F) {
        setLayout(1);
      }
      if (io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['2' - 96] && io.KeysDownDuration['2' - 96] == 0.F) {
        setLayout(2);
      }
      if (io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['3' - 96] && io.KeysDownDuration['3' - 96] == 0.F) {
        setLayout(3);
      }
      */
        

      if (buffer) {
        if (io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['v' - 96] && io.KeysDownDuration['v' - 96] == 0.F) {
          // mouse to screen
          ImVec2 m2s = io.MousePos - (w_pos + w_size) / 2.0F;
          // screen to grid
          ImVec2 s2g = m2s / w_scale - m_offset;

          paste(s2g);
        }
      }
      if (io.KeysDown[LIBSL_KEY_DELETE]) {
        for (std::shared_ptr<SelectableUI> item : selected) {
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
    std::shared_ptr<ProcessingGraph> currentGraph = m_graphs.top();

    // Draw visual comment
    for (std::shared_ptr<VisualComment> comment : currentGraph->comments()) {
      ImVec2 position = offset + comment->m_position * w_scale;
      ImGui::SetCursorPos(position);
      comment->draw();
    }

    // Draw the pipes
    float pipe_width = style.pipe_line_width * w_scale;
    int pipe_res = static_cast<int>(20 * w_scale);
    for (std::shared_ptr<Processor> processor : *currentGraph->processors()) {
      for (std::shared_ptr<ProcessorOutput> output : processor->outputs()) {
        for (std::shared_ptr<ProcessorInput> input : output->m_links) {
          ImVec2 A = input->getPosition()  + w_pos - ImVec2(pipe_width / 4.F, 0.F);
          ImVec2 B = output->getPosition() + w_pos + ImVec2(pipe_width / 4.F, 0.F);

          float dist = sqrt( (A-B).x * (A-B).x + (A-B).y * (A-B).y);

          ImVec2 bezier( (dist < 100.0F ? dist : 100.F) * w_scale, 0.0F);

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


          if (dist / w_scale > 125.0F && w_scale > 0.5F) {
            ImVec2 center = (A + B) / 2.F;
            ImVec2 vec = (A*2.F - bezier) - (B*2.F + bezier);
            ImVec2 norm_vec = (vec / sqrt(vec.x*vec.x + vec.y*vec.y)) * 1.5F*pipe_width;


            ImVec2 U = center + norm_vec * 1.5F;
            std::swap(norm_vec.x, norm_vec.y);
            ImVec2 V = center + norm_vec;
            ImVec2 W = center - norm_vec;
            std::swap(V.x, W.x);

            ImGui::GetWindowDrawList()->AddTriangleFilled(U, V, W, input->color());
          }
        }
      }
    }

    if (selected.size() == 1) { // ToDo : """this is a QUICK FIX""" Make this work for N nodes
      std::sort(
        currentGraph->processors()->begin(), currentGraph->processors()->end(),
        [](std::shared_ptr<Processor> /*p1*/, std::shared_ptr<Processor> p2) { return p2->m_selected; }
      );
    }

    // Draw the nodes
    for (std::shared_ptr<Processor> processor : *currentGraph->processors()) {
      ImVec2 position = offset + processor->m_position * w_scale;
      ImGui::SetCursorPos(position);
      processor->draw();
    }



    // Draw new pipe
    if (linking) {
      ImVec2 A = ImGui::GetMousePos();
      ImVec2 B = ImGui::GetMousePos();

      if (m_selected_input) {
        A = w_pos + m_selected_input->getPosition();
        A -= ImVec2(pipe_width / 4.F, 0.F);
      }
      else if (m_selected_output) {
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

    // Make sure the wheel is only io.MouseWheel, 1 and -1
    io.MouseWheel = io.MouseWheel > 0.001F ? 1.0F : io.MouseWheel < -0.001F ? -1.0F : 0.0F;

    float old_scale = window->FontWindowScale;
    float new_scale = ImClamp(
      window->FontWindowScale + c_zoom_motion_scale * io.MouseWheel * sqrt(old_scale),
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
  char search[64] = "";
  void NodeEditor::menus() {

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO io = ImGui::GetIO();

    float w_scale = window->FontWindowScale;

    window->FontWindowScale = 1.0F;

    // Draw menus
    if (Instance()->m_graph_menu) {
      ImGui::OpenPopup("graph_menu");
      search[0] = '\0';
    }
    if (Instance()->m_node_menu) {
      ImGui::OpenPopup("node_menu");
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
    if (ImGui::BeginPopup("graph_menu"))
    {

      ImVec2 w_pos = window->Pos;
      ImVec2 w_size = window->Size;

      // mouse to screen
      ImVec2 m2s = io.MousePos - (w_pos + w_size) / 2.0F;
      // screen to grid
      ImVec2 s2g = m2s / w_scale - Instance()->m_offset;

      Instance()->m_graph_menu = false;

      ImGui::InputTextWithHint("##", "search", search,  64);

      /*
      if (ImGui::BeginMenu("Add", "SPACE")) {
      */

      std::locale loc;
      for( auto& elem: search)
         elem = std::tolower(elem, loc);
      addNodeMenu(s2g, search);
      /*
        ImGui::EndMenu();
      }
      */

      if (buffer) {
        if (ImGui::MenuItem("Paste", "CTRL+V")) {
          // mouse to screen
          ImVec2 m2s = io.MousePos - (w_pos + w_size) / 2.0F;
          // screen to grid
          ImVec2 s2g = m2s / w_scale - m_offset;

          paste(s2g);
        }
      }
      /*
      if (ImGui::MenuItem("Add multiplexer node")) {
        std::shared_ptr<Multiplexer> proc = n_e->getCurrentGraph()->addProcessor<Multiplexer>();
        proc->setPosition(s2g);
      }
      

      if (ImGui::MenuItem("Comment")) {
        std::shared_ptr<VisualComment> com(new VisualComment());
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
        copy();
      }
      /*
      if (ImGui::MenuItem("Group"))
      {
        n_e->getCurrentGraph()->collapseSubset(selected);
      }
      if (ImGui::MenuItem("Ungroup")) {
        for (std::shared_ptr<SelectableUI> proc : selected) {
          if (typeid(*proc.raw()) == typeid(ProcessingGraph)) {
            n_e->getCurrentGraph()->expandGraph(std::shared_ptr<ProcessingGraph>(proc), proc->getPosition());
          }
        }
      }
      */
      if (ImGui::MenuItem("Delete")) {
        for (std::shared_ptr<SelectableUI> item : selected) {
          getCurrentGraph()->remove(item);
        }
      }
      if (ImGui::MenuItem("Unlink")) {
        for (std::shared_ptr<SelectableUI> select : selected) {
          std::shared_ptr<Processor> proc = std::static_pointer_cast<Processor>(select);
          if (proc) {
            for (std::shared_ptr<ProcessorInput> input : proc->inputs())
            {
              Processor::disconnect(input);
            }
            for (std::shared_ptr<ProcessorOutput> output : proc->outputs()) {
              Processor::disconnect(output);
            }
          }
        }
      }
      ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

    window->FontWindowScale = w_scale;
  }

  //-------------------------------------------------------
  void NodeEditor::selectProcessors() {
    m_selecting = true;

    NodeEditor* n_e = Instance();
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2  w_pos = window->Pos;
    ImVec2 w_size = window->Size;
    float w_scale = window->FontWindowScale;

    ImGuiIO io = ImGui::GetIO();

    ImVec2 A = (io.MousePos - (w_pos + w_size) / 2.0F) / w_scale - m_offset;
    ImVec2 B = (io.MouseClickedPos[0] - (w_pos + w_size) / 2.0F) / w_scale - m_offset;

    if (A.x > B.x) std::swap(A.x, B.x);
    if (A.y > B.y) std::swap(A.y, B.y);

    if (B.x - A.x > 10 || B.y - A.y > 10) {
      draw_list->AddRect(
        io.MouseClickedPos[0],
        io.MousePos,
        style.processor_selected_color
      );

      if (!io.KeysDown[LIBSL_KEY_SHIFT]) {
        for (std::shared_ptr<SelectableUI> selproc : selected) {
          selproc->m_selected = false;
        }
        selected.clear();
      }

      auto isInside = [](ImVec2 min, ImVec2 max, ImVec2& A, ImVec2& B) {
        return !(min.x < A.x || B.x < max.x || min.y < A.y || B.y < max.y);
      };

      for (std::shared_ptr<Processor> procui : *n_e->getCurrentGraph()->processors()) {
        ImVec2 pos_min = procui->getPosition();
        ImVec2 pos_max = pos_min + procui->m_size;
        procui->m_selected = isInside(pos_min, pos_max, A, B);
        if (procui->m_selected)
          selected.push_back(std::shared_ptr<SelectableUI>(procui));
      }

      for (std::shared_ptr<VisualComment> comui : n_e->getCurrentGraph()->comments()) {
        ImVec2 pos_min = comui->getPosition();
        ImVec2 pos_max = pos_min + comui->m_size;
        comui->m_selected = isInside(pos_min, pos_max, A, B);
        if (comui->m_selected)
          selected.push_back(std::shared_ptr<SelectableUI>(comui));
      }
    }
  }


  //-------------------------------------------------------
  void NodeEditor::copy() {
    buffer = getCurrentGraph()->copySubset(selected);
  }


  //-------------------------------------------------------
  void NodeEditor::paste(ImVec2 s2g) {
    std::shared_ptr<SelectableUI> copy_buff = buffer->clone();
    getCurrentGraph()->expandGraph(buffer, s2g);
    for (std::shared_ptr<SelectableUI> ui : *((std::shared_ptr<ProcessingGraph>)buffer)->processors())
    {
      ui->m_selected = true;
    }
    selected.clear();
    selected.push_back(buffer);
    buffer = std::static_pointer_cast<ProcessingGraph>(copy_buff);
  }


  //-------------------------------------------------------
  void NodeEditor::launchIcesl() {

    const char* icesl_path = Instance()->m_iceslPath.c_str();
    std::string icesl_params = " " + Instance()->m_iceSLTempExportPath.string();

#ifdef WIN32
    // CreateProcess init
    STARTUPINFO StartupInfo;
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof StartupInfo;

    ZeroMemory(&Instance()->m_icesl_p_info, sizeof(Instance()->m_icesl_p_info));

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
      &Instance()->m_icesl_p_info); // @lpProcessInformation - process info
    
    if (icesl_process) {
      // watch the process
      WaitForSingleObject(Instance()->m_icesl_p_info.hProcess, 1000);
      // getting the hwnd
      EnumWindows(EnumWindowsFromPid /*sets g_icesl_hwnd*/, Instance()->m_icesl_p_info.dwProcessId);
      sl_assert(Instance()->m_icesl_hwnd != NULL);
      if (Instance()->m_auto_export) {
        Instance()->exportIceSL(Instance()->m_iceSLTempExportPath);
      }
      // we give a default size, otherwise SetWindowLong gives unpredictable results (white window)
      MoveWindow(Instance()->m_icesl_hwnd, 0, 0, 800, 600, true);


    } else {
      // process creation failed
      std::cerr << Console::red << "Icesl couldn't be opened, please launch Icesl manually" << Console::gray << std::endl;
      std::cerr << Console::red << "ErrorCode: " << GetLastError() << Console::gray << std::endl;

      Instance()->m_icesl_hwnd = NULL;
    }
#elif __linux__
  std::system( (icesl_path + icesl_params + " &").c_str() );
#endif
  }

  //-------------------------------------------------------
  void NodeEditor::closeIcesl() {
#ifdef WIN32
    if (s_instance->m_icesl_p_info.hProcess == NULL) {
      std::cerr << Console::red << "Icesl Handle not found. Please close Icesl manually." << Console::gray << std::endl;
    }
    else {
      // close all windows with icesl's pid
      EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)s_instance->m_icesl_p_info.dwProcessId);

      // check if handle still responds, else handle is killed
      if (WaitForSingleObject(s_instance->m_icesl_p_info.hProcess, 1000) != WAIT_OBJECT_0) {
        TerminateProcess(s_instance->m_icesl_p_info.hProcess, 0);
        std::cerr << Console::yellow << "Trying to force close Icesl." << Console::gray << std::endl;
      }
      else {
        std::cerr << Console::yellow << "Icesl was successfully closed." << Console::gray << std::endl;
      }
      CloseHandle(s_instance->m_icesl_p_info.hProcess);
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
        "function data(name, type, ...)" << std::endl <<
        "  return __input[name][1]" << std::endl <<
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
    std::string filename(ChillFolder() + m_settingsFileName);
    //filename += g_settingsFileName;

    // remove space from icesl path for storage in txt file
    std::string cleanedIceslPath = m_iceslPath;
    std::replace(cleanedIceslPath.begin(), cleanedIceslPath.end(), ' ', '#');

    std::ofstream f(filename);
    f << "icesl_path " << cleanedIceslPath << std::endl;
    f << "auto_save " << m_auto_save << std::endl;
    f << "auto_export " << m_auto_export << std::endl;
    f << "auto_launch_icesl " << m_auto_icesl << std::endl;
    f << "icesl_is_docked " << m_icesl_is_docked << std::endl;
    f << "ratio_iceslx " << m_ratio_icesl.x << std::endl;
    f << "ratio_icesly " << m_ratio_icesl.y << std::endl;
    f << "offset_iceslx " << m_offset_icesl.x << std::endl;
    f << "offset_icesly " << m_offset_icesl.y << std::endl;
    f.close();
  }

  //-------------------------------------------------------
  void NodeEditor::loadSettings()
  {
    std::string filename = ChillFolder() + m_settingsFileName;
    if (LibSL::System::File::exists(filename.c_str())) {
      std::ifstream f(filename);
      while (!f.eof()) {
        std::string setting, value, raw;
        f >> setting >> value;
        if (setting == "icesl_path") {
          std::replace(value.begin(), value.end(), '#', ' ');
          m_iceslPath = value;
        }
        if (setting == "auto_save") {
          m_auto_save = (std::stoi(value) ? true : false);
        }
        if (setting == "auto_export") {
          m_auto_export = (std::stoi(value) ? true : false);
        }
        if (setting == "auto_launch_icesl") {
          m_auto_icesl = (std::stoi(value) ? true : false);
        }
        if (setting == "icesl_is_docked") {
          m_icesl_start_docked = (std::stoi(value) ? true : false);
        }
        if (setting == "ratio_iceslx") {
          m_ratio_icesl.x = (std::stof(value));
        }
        if (setting == "ratio_icesly") {
          m_ratio_icesl.y = (std::stof(value));
        }
        if (setting == "offset_iceslx") {
          m_offset_icesl.x = (std::stof(value));
        }
        if (setting == "offset_icesly") {
          m_offset_icesl.y = (std::stof(value));
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
    nodeEditor->exportIceSL(Instance()->m_iceSLTempExportPath.string());

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

      if (nodeEditor->m_auto_icesl) {
#ifdef WIN32
        SimpleUI::setCustomCallbackMsgProc(custom_wndProc);

        // get app handler
        Instance()->m_chill_hwnd = SimpleUI::getHWND();
#endif
        // launching Icesl
        launchIcesl();

        // place apps in default pos
        nodeEditor->setDefaultAppsPos();
      }

      // main loop
      SimpleUI::loop();

      if (nodeEditor->m_auto_icesl) {
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

  void NodeEditor::setDefaultAppsPos() {
    if (m_icesl_start_docked) {
      dock();
    } else {
      undock();
    }

    #ifdef WIN32
    // start maximized
    ShowWindow(m_chill_hwnd, SW_SHOWMAXIMIZED);
    #endif
  }

  //-------------------------------------------------------

  void NodeEditor::moveIceSLWindowAlongChill(bool preserve_ratio,bool set_chill_full_width) {
#ifdef WIN32

    static bool already_entered = false;

    if (already_entered) {
      return; // prevent windows re-entrant call on MoveWindow
    }
    already_entered = true;

    if (m_icesl_hwnd != NULL) {

      const int magic_x_offset = 15; // TODO determine automatically?

      if (m_icesl_is_docked) {

        // get current windows dimensions
        RECT chillRect;
        GetWindowRect(m_chill_hwnd, &chillRect);

        RECT iceslRect;
        GetWindowRect(m_icesl_hwnd, &iceslRect);

        if (set_chill_full_width) {
          chillRect.right = iceslRect.right;
          MoveWindow(m_chill_hwnd, chillRect.left, chillRect.top, chillRect.right - chillRect.left, chillRect.bottom - chillRect.top, true);
        }

        int icesl_xpos = chillRect.left + m_offset_icesl.x * (chillRect.right - chillRect.left) - magic_x_offset;
        int icesl_ypos = chillRect.top + m_offset_icesl.y * (chillRect.bottom - chillRect.top);
        int icesl_width = (chillRect.right - chillRect.left) * m_ratio_icesl.x + 3;
        int icesl_height = (chillRect.bottom - chillRect.top) * m_ratio_icesl.y;

        // snap IceSL window to Chill, preserve width
        BringWindowToTop(m_chill_hwnd);
        MoveWindow(m_icesl_hwnd, icesl_xpos, icesl_ypos, icesl_width, icesl_height, true);
        SetWindowPos(m_chill_hwnd, m_icesl_hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

      } else {

        // get current windows dimensions
        RECT chillRect;
        GetWindowRect(m_chill_hwnd, &chillRect);

        RECT iceslRect;
        GetWindowRect(m_icesl_hwnd, &iceslRect);

        int total_width = max(iceslRect.right, chillRect.right) - chillRect.left + magic_x_offset;
        if (total_width - magic_x_offset*2 < (chillRect.right - chillRect.left)) {
          // icesl does not extend beyond the right border of chill, we have to resize chill with the chosen ratio
          // NOTE: this happens when un-docking
          MoveWindow(m_chill_hwnd,
            chillRect.left,
            chillRect.top,
            total_width / 2,
            chillRect.bottom - chillRect.top, true);
          MoveWindow(m_icesl_hwnd,
            chillRect.left + total_width / 2 - magic_x_offset,
            chillRect.top,
            total_width / 2 + magic_x_offset,
            chillRect.bottom - chillRect.top, true);
        } else {
          // snap IceSL window to Chill, preserve width
          MoveWindow(m_icesl_hwnd,
            chillRect.right - magic_x_offset,
            chillRect.top,
            preserve_ratio ? (iceslRect.right - iceslRect.left) : total_width - (chillRect.right - chillRect.left),
            chillRect.bottom - chillRect.top, true);
        }
      }

    }

    already_entered = false;

#endif
  }
  
  //-------------------------------------------------------
  bool NodeEditor::scriptPath(const std::string& name, std::string& _path)
  {
    std::string         path;
    std::vector<std::string> paths;

#ifdef WIN32
    paths.push_back(getenv("APPDATA") + std::string("\\ChiLL") + name);
#elif __linux__
    paths.push_back("/etc/chill" + name);
#endif
    paths.push_back(fs::current_path().string() + name);
    paths.push_back(".." + name);
    paths.push_back("../.." + name);
    paths.push_back("../../.." + name);
    paths.push_back("../../../.." + name);
    paths.push_back(SRC_PATH + name); // dev path
    
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
    if (m_iceslPath.empty()) {
#ifdef WIN32
      m_iceslPath = getenv("PROGRAMFILES") + std::string("/INRIA/IceSL/bin/IceSL-slicer.exe");
#elif __linux__
      m_iceslPath = getenv("HOME") + std::string("/icesl/bin/IceSL-slicer");
#endif
    }
    if (!LibSL::System::File::exists(m_iceslPath.c_str())) {
      std::cerr << Console::red << "IceSL executable not found. Please specify the location of IceSL's executable." << Console::gray << std::endl;

      const char * modalTitle = "IceSL was not found...";
      const char * modalText = "Icesl was not found on this computer.\n\nIn order for Chill to work properly, it needs access to IceSL.\n\nPlease specify the location of IceSL's executable on your computer.";

#ifdef WIN32
      uint modalFlags = MB_OKCANCEL | MB_DEFBUTTON1 | MB_SYSTEMMODAL | MB_ICONINFORMATION;

      int modal = MessageBox(m_chill_hwnd, modalText, modalTitle, modalFlags);

      if (modal == 1) {
        m_iceslPath = openFileDialog(OFD_FILTER_ALL).c_str();
        std::cerr << Console::yellow << "IceSL location specified: " << m_iceslPath << Console::gray << std::endl;
      }
      else {
        m_iceslPath = "";
      }

#elif __linux__
      // TODO Linux modal window
      //std::system( ("echo -e " + std::string(modalText) + " | xmessage -file -").c_str() );
      std::system( ("xmessage \""+ std::string(modalText) + "\" -title \"" + std::string(modalTitle) + "\"").c_str() );
      m_iceslPath = openFileDialog(OFD_FILTER_ALL).c_str();
      std::cerr << Console::yellow << "IceSL location specified: " << m_iceslPath << Console::gray << std::endl;
#endif
    }
  }


  void NodeEditor::updateIceSLPosRatio() {
#ifdef WIN32
    if (m_icesl_is_docked) {
      if (!m_minimized && m_layout == 3) {
        RECT rect_icesl;
        RECT rect_chill;
        if (GetWindowRect(m_icesl_hwnd, &rect_icesl) && GetWindowRect(m_chill_hwnd, &rect_chill)) {
          int width_icesl = rect_icesl.right - rect_icesl.left;
          int height_icesl = rect_icesl.bottom - rect_icesl.top;
          int width_chill = rect_chill.right - rect_chill.left;
          int height_chill = rect_chill.bottom - rect_chill.top;

          m_ratio_icesl.x = float(width_icesl) / float(width_chill);
          m_ratio_icesl.y = float(height_icesl) / float(height_chill);

          m_offset_icesl.x = float(rect_icesl.left - rect_chill.left) / float(width_chill);
          m_offset_icesl.y = float(rect_icesl.top - rect_chill.top) / float(height_chill);
        }
      }
    }
#endif
  }

  void NodeEditor::showIceSL() {
#ifdef WIN32
    if (m_icesl_is_docked) {
      if (m_icesl_hwnd != nullptr) {
        SetWindowPos(m_chill_hwnd, m_icesl_hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
      }
    }
#endif
  }

  void NodeEditor::setLayout(int l) {
#ifdef WIN32
    if (l < sizeof(layouts) / sizeof(layouts[0]) ) {
      m_offset_icesl = layouts[l].offset_icesl;
      m_ratio_icesl = layouts[l].ratio_icesl;
      m_layout = l;
    }
#endif
  }

  void NodeEditor::dock() {
#ifdef WIN32
    bool set_chill_full_width = !m_icesl_is_docked;
    m_icesl_is_docked = true;
    SetWindowLongPtr(m_icesl_hwnd, GWL_STYLE, (GetWindowLongPtr(m_icesl_hwnd, GWL_STYLE) | WS_CHILD) & ~WS_POPUPWINDOW & ~WS_SIZEBOX & ~WS_CAPTION);
    SetWindowLongPtr(m_chill_hwnd, GWL_STYLE, GetWindowLongPtr(m_chill_hwnd, GWL_STYLE) | WS_CLIPCHILDREN);
    setLayout(m_layout);
    moveIceSLWindowAlongChill(true, set_chill_full_width);
#endif 
  }

  void NodeEditor::undock() {
#ifdef WIN32
    m_icesl_is_docked = false;
    SetWindowLongPtr(m_icesl_hwnd, GWL_STYLE, (GetWindowLongPtr(m_icesl_hwnd, GWL_STYLE) & ~WS_CHILD & ~WS_SIZEBOX) | WS_POPUPWINDOW | WS_CAPTION);
    SetWindowLongPtr(m_chill_hwnd, GWL_STYLE, GetWindowLongPtr(m_chill_hwnd, GWL_STYLE) & ~WS_CLIPCHILDREN);
    moveIceSLWindowAlongChill(true,false);
#endif 
  }

  void NodeEditor::maximize()
  {
    #ifdef WIN32
    // get current monitor info (set to zero in not specified, eg. hMonitor == NULL)
    /*
    HMONITOR hMonitor = MonitorFromWindow(NodeEditor::Instance()->m_chill_hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitorInfo;
    memset(&monitorInfo, 0, sizeof(MONITORINFO));
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (hMonitor != NULL) {
      GetMonitorInfo(hMonitor, &monitorInfo);;
    }
    */

    // get desktop dimmensions
    int desktop_width, desktop_height = 0;
    getDesktopRes(desktop_width, desktop_height);

    if (m_icesl_is_docked) {
      MoveWindow(m_chill_hwnd, 0, 0, desktop_width, desktop_height, true);
      moveIceSLWindowAlongChill(true,false);
    } else {
      const int magic_x_offset = 15; // TODO determine automatically?
      MoveWindow(m_chill_hwnd, 0, 0, desktop_width / 2, desktop_height, true);
      MoveWindow(m_icesl_hwnd, desktop_width / 2 - magic_x_offset, 0, desktop_width / 2 + magic_x_offset, desktop_height, true);
      SetActiveWindow(m_chill_hwnd);
    }
#endif
  }
}
