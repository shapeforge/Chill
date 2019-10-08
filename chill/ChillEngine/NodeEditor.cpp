#include "NodeEditor.h"

#include "SDL_pixels.h"

//#include <gl/GL.h>
// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

#include "Processor.h"
#include "LuaProcessor.h"
#include "IOs.h"
#include "GraphSaver.h"
#include "FileManager.h"
#include "VisualComment.h"

#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"


namespace chill
{
#undef ForIndex
#define ForIndex(I, N) for(decltype(N) I=0; I<N; I++)

  NodeEditor* NodeEditor::s_instance = nullptr;

  //---------------------------------------------------
  fs::path recursiveFileSelecter(const fs::path* _current_dir, const std::vector<const char*>* _filter, bool isMenu = true)
  {
    std::vector<fs::path> files;
    std::vector<fs::path> directories;

    files = listFileInDir(_current_dir, _filter);
    directories = listFolderinDir(_current_dir);

    fs::path nameDir;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7F, 0.7F, 1.0F, 1.0F));
    for (auto dir : directories) {
      if (!nameDir.empty()) break;

      if (!isHidden(dir)) {
        fs::path currdir(_current_dir->generic_string());
        currdir /= dir;

        if (!isMenu && ImGui::CollapsingHeader((dir.filename().generic_string() + "##" + _current_dir->generic_string()).c_str() )) {
          ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10);
          ImGui::BeginGroup();
          nameDir = recursiveFileSelecter(&currdir, _filter, isMenu);
          ImGui::EndGroup();
        }

        if (isMenu && ImGui::BeginMenu(dir.filename().generic_string().c_str())) {
          nameDir = recursiveFileSelecter(&currdir, _filter, isMenu);
          ImGui::EndMenu();
        }
      }
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
    for (auto file : files) {
      
      std::string name = fs::path(file).replace_extension().filename().generic_string();
 
      if (ImGui::MenuItem(name.c_str())) {
        nameDir = file;
      }

    }
    ImGui::PopStyleColor();
    return nameDir;
  }

  //-------------------------------------------------------
  bool addNodeMenu(ImVec2 _pos, bool isMenu = true) {
    NodeEditor* n_e = NodeEditor::Instance();
    fs::path nodeFolder = (getUserDir() /= "chill-nodes");
    fs::path node = recursiveFileSelecter( &nodeFolder, &OFD_FILTER_NODES, isMenu);
    if (!node.empty()) {
      std::shared_ptr<LuaProcessor> proc = n_e->getCurrentGraph()->addProcessor<LuaProcessor>(relative(&node, &nodeFolder));
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
#endif

  //-------------------------------------------------------

  void NodeEditor::loadGraph(const fs::path* _path, bool _setAsAutoSavePath = false) {
    GraphSaver loader;
    loader.execute(_path);

    if (_setAsAutoSavePath) {
      m_graphPath = fs::path(*_path);
    }


    // Recenter the view and adjust zoom
    auto bbox = getMainGraph()->getBoundingBox();
    ImVec2 center = bbox.center();

    ImGuiWindow* window = m_graphWindow;

    if (window) {
      m_offset = center - window->Pos;
      m_zoom =  std::min(window->Size.x / bbox.extent().x, window->Size.y / bbox.extent().y) * 0.8F;
    }
  }


  //-------------------------------------------------------

  NodeEditor::NodeEditor() {
    m_graphs.push(std::shared_ptr<ProcessingGraph>(new ProcessingGraph()));
  }

  //-------------------------------------------------------

  NodeEditor* NodeEditor::Instance() {
    if (!s_instance) {
      s_instance = new NodeEditor();

      fs::path filepath = (getUserDir() /= "chill-nodes") /= "init.graph";
      if (fs::exists(filepath)) {
        s_instance->loadGraph(&filepath);
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
      //TODO HWND hWND = SimpleUI::getHWND();
      HWND hWND = NULL;
      HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(16001));
      SendMessage(hWND, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
      SendMessage(hWND, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
#endif
      icon_changed = true;
    }
    // TODO: linux icon load
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

    return true;
  }

  //-------------------------------------------------------
  void NodeEditor::drawMenuBar()
  {
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, style.menubar_color);

    if (ImGui::BeginMainMenuBar()) {
      // opening file
      fs::path fullpath;
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New graph")) {
          fs::path graph_filename = getMainGraph()->name() + ".graph";
          fullpath = saveFileDialog(&graph_filename, &OFD_FILTER_GRAPHS);
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
          fullpath = openFileDialog(&OFD_FILTER_GRAPHS);
          if (!fullpath.empty()) {
            const fs::path path = fs::path(fullpath);
            loadGraph(&path, true);
          }
        }
        if (ImGui::MenuItem("Load example graph")) {
          std::string folderName = "chill-models";

          fs::path modelsFolder = getUserDir() /= folderName;
          if (fs::exists(modelsFolder)) {
            fullpath = openFileDialog(&modelsFolder, &OFD_FILTER_GRAPHS);
            if (!fullpath.empty()) {
              loadGraph(&fullpath);
            }
          }
        }
        if (ImGui::MenuItem("Save graph", "Ctrl+S")) {
          fs::path graph_filename = getMainGraph()->name() + ".graph";
          fullpath = saveFileDialog(&graph_filename, &OFD_FILTER_GRAPHS);
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
          fs::path graph_filename = getCurrentGraph()->name() + ".graph";
          fullpath = saveFileDialog(&graph_filename, &OFD_FILTER_GRAPHS);
          if (!fullpath.empty()) {
            std::ofstream file;
            file.open(fullpath);
            getCurrentGraph()->save(file);
            file.close();
          }
        }
        */

        if (ImGui::MenuItem("Export to IceSL lua")) {
          fs::path graph_filename = getMainGraph()->name() + ".lua";
          m_iceSLExportPath = saveFileDialog(&graph_filename, &OFD_FILTER_LUA);
          exportIceSL(&m_iceSLExportPath);
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Settings")) {
        ImGui::MenuItem("Automatic save", "", &m_auto_save);
        ImGui::MenuItem("Automatic export", "", &m_auto_export);
        ImGui::MenuItem("Automatic use of IceSL", "", &m_auto_icesl);
        ImGui::EndMenu();
      }

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

    ImVec2 s2g = ImVec2(-100, -50) - Instance()->m_offset;

    ImGui::InputTextWithHint("##", "search", leftMenuSearch,  64);

    std::locale loc;
    for( auto& elem: leftMenuSearch)
       elem = std::tolower(elem, loc);
    addNodeMenu(s2g, false);

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

    ImGuiWindow* window = m_graphWindow = ImGui::GetCurrentWindow();
    ImGuiIO      io     = ImGui::GetIO();

    ImVec2 w_pos  = window->Pos;
    ImVec2 w_size = window->Size;
    window->FontWindowScale = m_zoom;

    ImVec2 offset = (m_offset * m_zoom + (w_size - w_pos) / 2.0F);

    if (ImGui::IsWindowHovered()) {
      // clear data
      hovered.clear();
      selected.clear();
      text_editing = false;

      for (std::shared_ptr<Processor> processor : *m_graphs.top()->processors()) {
        ImVec2 socket_size = ImVec2(1, 1) * (style.socket_radius + style.socket_border_width) * m_zoom;

        ImVec2 size = processor->m_size * m_zoom;
        ImVec2 min_pos = offset + w_pos + processor->m_position * m_zoom - socket_size;
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
        ImVec2 socket_size = ImVec2(1, 1) * (style.socket_radius + style.socket_border_width) * m_zoom;

        ImVec2 size = comment->m_title_size * m_zoom;
        ImVec2 min_pos = offset + w_pos + comment->m_position * m_zoom - socket_size;
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
        if (!io.KeysDown[0]) { // LIBSL_KEY_CTRL
          for (std::shared_ptr<SelectableUI> object : selected) {
            object->translate(io.MouseDelta / m_zoom);
          }
        }
        else {
          for (std::shared_ptr<SelectableUI> object : selected) {
            std::shared_ptr<VisualComment> com = std::static_pointer_cast<VisualComment>(object);
            if (com)
              object->m_size += io.MouseDelta / m_zoom;
          }
        }
      }

      // Move the whole canvas
      if (io.MouseDown[2] && ImGui::IsWindowHovered()) {
        m_offset += io.MouseDelta / m_zoom;
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
        

      

      // update the offset used for display
      offset = (m_offset * m_zoom + (w_size - w_pos) / 2.0F);
    } // ! if window hovered

    // Draw the grid
    if (m_show_grid)
    {
      drawGrid();
    }
    std::shared_ptr<ProcessingGraph> currentGraph = m_graphs.top();

    // Draw visual comment
    for (std::shared_ptr<VisualComment> comment : currentGraph->comments()) {
      ImVec2 position = offset + comment->m_position * m_zoom;
      ImGui::SetCursorPos(position);
      comment->draw();
    }

    // Draw the pipes
    float pipe_width = style.pipe_line_width * m_zoom;
    int pipe_res = static_cast<int>(20 * m_zoom);
    for (std::shared_ptr<Processor> processor : *currentGraph->processors()) {
      for (std::shared_ptr<ProcessorOutput> output : processor->outputs()) {
        for (std::shared_ptr<ProcessorInput> input : output->m_links) {
          ImVec2 A = input->getPosition()  + w_pos - ImVec2(pipe_width / 4.F, 0.F);
          ImVec2 B = output->getPosition() + w_pos + ImVec2(pipe_width / 4.F, 0.F);

          float dist = sqrt( (A-B).x * (A-B).x + (A-B).y * (A-B).y);

          ImVec2 bezier( (dist < 100.0F ? dist : 100.F) * m_zoom, 0.0F);

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


          if (dist / m_zoom > 125.0F && m_zoom > 0.5F) {
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
      ImVec2 position = offset + processor->m_position * m_zoom;
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

      ImVec2 bezier(100.0F * m_zoom, 0.0F);
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





    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

  


    bool wasDirty = false;
    for (std::shared_ptr<Processor> processor : *m_graphs.top()->processors()) {
      if (processor->isDirty()) {
        wasDirty = true;
        processor->setDirty(false);
      }
    }

    if (wasDirty) {
      modify();
      if (m_auto_export) {
        exportIceSL(&m_iceSLTempExportPath);
      }

      if (m_auto_save) {
        std::ofstream file;
        file.open(m_graphPath);
        m_graphs.top()->save(file);
      }
    }
    
    
    window->FontWindowScale = 1.0F;
    //DO NOT CHANGE ORDER OF THE FOLLOWING TWO FUNCTIONS !
    shortcutsAction();
    menus();
    window->FontWindowScale = m_zoom;

    ImGui::End(); // Graph
  }

  //-------------------------------------------------------
  void NodeEditor::zoom() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiIO io = ImGui::GetIO();

    // Make sure the wheel is only io.MouseWheel, 1 and -1
    io.MouseWheel = io.MouseWheel > 0.001F ? 1.0F : io.MouseWheel < -0.001F ? -1.0F : 0.0F;

    float old_scale = m_zoom;
    float new_scale = ImClamp(
      m_zoom + c_zoom_motion_scale * io.MouseWheel * sqrt(old_scale),
      0.1F,
      2.0F);

    // ToDo : Move this elsewhere !!
    if (old_scale <= 0.1F && io.MouseWheel < 0 && m_graphs.size() > 1) {
      m_graphs.pop();
      new_scale = 2.0F;
    }

    m_zoom = new_scale;

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

    ImVec2 offset = (m_offset * m_zoom + (window->Size - window->Pos) / 2.F);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const ImU32& grid_Color      = style.graph_grid_color;
    const float& grid_Line_width = style.graph_grid_line_width;

    const int subdiv = m_zoom >= 1.0F ? 10 : 100;

    int grid_size  = static_cast<int>(m_zoom * subdiv);
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
      ImVec2 s2g = m2s / m_zoom - Instance()->m_offset;

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
          paste();
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
  }

  //-------------------------------------------------------
  void NodeEditor::selectProcessors() {
    m_selecting = true;

    NodeEditor* n_e = Instance();
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2  w_pos = window->Pos;
    ImVec2 w_size = window->Size;

    ImGuiIO io = ImGui::GetIO();

    ImVec2 A = (io.MousePos - (w_pos + w_size) / 2.0F) / m_zoom - m_offset;
    ImVec2 B = (io.MouseClickedPos[0] - (w_pos + w_size) / 2.0F) / m_zoom - m_offset;

    if (A.x > B.x) std::swap(A.x, B.x);
    if (A.y > B.y) std::swap(A.y, B.y);

    if (B.x - A.x > 10 || B.y - A.y > 10) {
      draw_list->AddRect(
        io.MouseClickedPos[0],
        io.MousePos,
        style.processor_selected_color
      );

      if (!io.KeysDown[0]) { // TODO LIBSL_KEY_SHIFT
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
  void NodeEditor::shortcutsAction() {
    ImGuiIO      io = ImGui::GetIO();

    //ctrl + c

    //ctrl + d docking

    // crtl + v

    //del 

    //undo redo

    if (io.MouseClicked[0]) {
      // if no processor hovered, clear
      if (!io.KeysDown[0] && hovered.empty()) { // LIBSL_KEY_CTRL
        for (std::shared_ptr<SelectableUI> selproc : selected) {
          selproc->m_selected = false;
        }
        selected.clear();
      }
      else {
        for (std::shared_ptr<SelectableUI> hovproc : hovered) {
          if (io.KeysDown[0]) { // LIBSL_KEY_CTRL
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

  }


  //-------------------------------------------------------
  void NodeEditor::copy() {
    buffer = getCurrentGraph()->copySubset(selected);
  }


  //-------------------------------------------------------
  void NodeEditor::paste() {
    ImGuiIO      io = ImGui::GetIO();
    ImGuiWindow* window =m_graphWindow;
    ImVec2 w_pos = window->Pos;
    ImVec2 w_size = window->Size;

    // mouse to screen
    ImVec2 m2s = io.MousePos - (w_pos + w_size) / 2.0F;
    // screen to grid
    ImVec2 s2g = m2s / m_zoom - m_offset;

    std::shared_ptr<SelectableUI> copy_buff = buffer->clone();
    getCurrentGraph()->expandGraph(buffer, s2g);
    for (std::shared_ptr<SelectableUI> selproc : selected) {
      selproc->m_selected = false;
    }
    selected.clear();
    for (std::shared_ptr<SelectableUI> ui : *((std::shared_ptr<ProcessingGraph>)buffer)->processors())
    {
      ui->m_selected = true;
    }
    selected.push_back(buffer);
    buffer = std::static_pointer_cast<ProcessingGraph>(copy_buff);
  }

  //-------------------------------------------------------
  void NodeEditor::undo() {
    if (m_undo.size()>1) {
      m_redo.push_back(m_undo.back());
      m_undo.pop_back();
      std::shared_ptr<ProcessingGraph> graph = m_undo.back();
      setMainGraph(graph);
    }
  }

  //-------------------------------------------------------
  void NodeEditor::redo() {
    if (m_redo.size() > 0) {
      m_undo.push_back(m_redo.back());
      std::shared_ptr<ProcessingGraph> graph = m_redo.back();
      m_redo.pop_back();
      setMainGraph(graph);
    }
  }

  //-------------------------------------------------------
  void NodeEditor::modify() {
    m_redo.clear();
    // duplication du graph
    std::shared_ptr<ProcessingGraph> duplicate = std::static_pointer_cast<ProcessingGraph> (getMainGraph()->clone());
    for (std::shared_ptr<Processor> processor : *duplicate->processors()) {
      if (processor->isDirty()) {
        processor->setDirty(false);
      }
    }
    m_undo.push_back(duplicate);
  }

  //-------------------------------------------------------
  void NodeEditor::launchIcesl() {

    fs::path icesl_path = Instance()->m_iceslPath;
    fs::path icesl_params = " ";
    icesl_params += Instance()->m_iceSLTempExportPath;

#ifdef WIN32
    // CreateProcess init
    STARTUPINFO StartupInfo;
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof StartupInfo;

    ZeroMemory(&Instance()->m_icesl_p_info, sizeof(Instance()->m_icesl_p_info));

    // create the process
    auto icesl_process = CreateProcess(icesl_path.string().c_str(), // @lpApplicationName - application name / path 
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
      assert(Instance()->m_icesl_hwnd != NULL);
      if (Instance()->m_auto_export) {
        Instance()->exportIceSL(&(Instance()->m_iceSLTempExportPath));
      }
      // we give a default size, otherwise SetWindowLong gives unpredictable results (white window)
      //MoveWindow(Instance()->m_icesl_hwnd, 0, 0, 800, 600, true);

      //Instance()->m_icesl_window = SDL_CreateWindowFrom(Instance()->m_icesl_hwnd);

     /* SDL_SysWMinfo info_before;
      SDL_GetWindowWMInfo(Instance()->m_icesl_window, &info_before);
      SDL_DestroyWindow(Instance()->m_icesl_window);*/

      //SDL_DestroyWindow(Instance()->m_icesl_window);
      //SDL_Renderer *renderer;
      //SDL_CreateWindowAndRenderer(0,0, SDL_WINDOW_ALWAYS_ON_TOP, &Instance()->m_icesl_window, &renderer);
      /*SDL_SysWMinfo info_after;
      SDL_GetWindowWMInfo(Instance()->m_icesl_window, &info_after);
      info_after = info_before;*/

    } else {
      // process creation failed
      std::cerr << "Icesl couldn't be opened, please launch Icesl manually" << std::endl;
      std::cerr << "ErrorCode: " << GetLastError() << std::endl;

      Instance()->m_icesl_hwnd = NULL;
    }
#elif __linux__
    icesl_path += icesl_params;
    icesl_path += " &";
    std::system(icesl_path.string().c_str());
#endif
  }

  //-------------------------------------------------------
  void NodeEditor::closeIcesl() {
#ifdef WIN32
    if (s_instance->m_icesl_p_info.hProcess == NULL) {
      std::cerr << "Icesl Handle not found. Please close Icesl manually." << std::endl;
    }
    else {
      // close all windows with icesl's pid
      EnumWindows((WNDENUMPROC)TerminateAppEnum, (LPARAM)s_instance->m_icesl_p_info.dwProcessId);

      // check if handle still responds, else handle is killed
      if (WaitForSingleObject(s_instance->m_icesl_p_info.hProcess, 1000) != WAIT_OBJECT_0) {
        TerminateProcess(s_instance->m_icesl_p_info.hProcess, 0);
        std::cerr << "Trying to force close Icesl." << std::endl;
      }
      else {
        std::cerr << "Icesl was successfully closed." << std::endl;
      }
      CloseHandle(s_instance->m_icesl_p_info.hProcess);
    }
#endif
  }

  //-------------------------------------------------------
  void NodeEditor::exportIceSL(const fs::path* filename) {
    if (!filename->empty()) {
      std::ofstream file;
      file.open(*filename);

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
    fs::path filename(getUserDir() += m_settingsFileName);

    // remove space from icesl path for storage in txt file
    std::string cleanedIceslPath = m_iceslPath.string();
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
    fs::path filename = getUserDir() += m_settingsFileName;
    if (fs::exists(filename.c_str())) {
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
  void NodeEditor::getDesktopRes(int& width, int& height) {
    SDL_Rect desktop;

    int current_display = SDL_GetWindowDisplayIndex(Instance()->m_chill_window);

    SDL_GetDisplayUsableBounds(current_display, &desktop);

    width = desktop.w;
    height = desktop.h;
  }

  //-------------------------------------------------------
  int NodeEditor::launch()
  {
    // Setup SDL
    //if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
      printf("Error: %s\n", SDL_GetError());
      return -1;
    }

    NodeEditor *nodeEditor = Instance();

    nodeEditor->loadSettings();
    nodeEditor->SetIceslPath();

    // create the temp file
    nodeEditor->exportIceSL(&(Instance()->m_iceSLTempExportPath));

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    nodeEditor->m_chill_window = SDL_CreateWindow("Chill, the node-based editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, nodeEditor->default_width, nodeEditor->default_height, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(nodeEditor->m_chill_window);
    SDL_GL_MakeCurrent(nodeEditor->m_chill_window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
      fprintf(stderr, "Failed to initialize OpenGL loader!\n");
      return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(nodeEditor->m_chill_window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

    if (nodeEditor->m_auto_icesl) {
      SDL_SysWMinfo wmInfo;
      SDL_VERSION(&wmInfo.version);
      SDL_GetWindowWMInfo(nodeEditor->m_chill_window, &wmInfo);
#if WIN32
      Instance()->m_chill_hwnd = wmInfo.info.win.window;
#endif

      // launching Icesl
      launchIcesl();

      SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
      
      SDL_Renderer* chill_renderer = SDL_CreateRenderer(nodeEditor->m_chill_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

      nodeEditor->m_icesl_window = SDL_CreateWindowFrom(nodeEditor->m_icesl_hwnd);
      SDL_Renderer* icesl_renderer = SDL_CreateRenderer(nodeEditor->m_icesl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

      SDL_RenderClear(chill_renderer);
      SDL_RenderPresent(chill_renderer);

      SDL_RenderClear(icesl_renderer);
      SDL_RenderPresent(icesl_renderer);
      

      // place apps in default pos
      nodeEditor->setDefaultAppsPos();
    }

    // Main loop
    bool done = false;
    while (!done)
    {
      // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
        // TODO implement events 
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
          done = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(nodeEditor->m_chill_window))
          done = true;
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED && event.window.windowID == SDL_GetWindowID(nodeEditor->m_chill_window)) {
          int w, h;
          SDL_GetWindowSize(nodeEditor->m_chill_window, &w, &h);
          Instance()->m_size = ImVec2(static_cast<float>(w), static_cast<float>(h));
          Instance()->moveIceSLWindowAlongChill();
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SHOWN && event.window.windowID == SDL_GetWindowID(nodeEditor->m_chill_window)) {
          Instance()->m_size = ImVec2(static_cast<float>(nodeEditor->default_width), static_cast<float>(nodeEditor->default_height));
          Instance()->moveIceSLWindowAlongChill();
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_MOVED && event.window.windowID == SDL_GetWindowID(nodeEditor->m_chill_window)) {
          int w, h;
          SDL_GetWindowSize(nodeEditor->m_chill_window, &w, &h);
          Instance()->m_size = ImVec2(static_cast<float>(w), static_cast<float>(h));
          Instance()->moveIceSLWindowAlongChill();
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_MAXIMIZED && event.window.windowID == SDL_GetWindowID(nodeEditor->m_chill_window)) {
          nodeEditor->maximize();
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED && event.window.windowID == SDL_GetWindowID(nodeEditor->m_chill_window)) {
          nodeEditor->raiseIceSL();
        }
      }

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame(nodeEditor->m_chill_window);
      ImGui::NewFrame();

      NodeEditor::mainRender();

      glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
      glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      SDL_GL_SwapWindow(nodeEditor->m_chill_window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(nodeEditor->m_chill_window);
    SDL_Quit();

    if (nodeEditor->m_auto_icesl) {
      // closing Icesl
      std::atexit(closeIcesl);
    }

    nodeEditor->saveSettings();

    return 0;
  }

  //-------------------------------------------------------

  void NodeEditor::setDefaultAppsPos() {
    if (m_icesl_start_docked) {
      dock();
    } else {
      undock();
    }

    // start maximized
    NodeEditor::maximize();
  }

  //-------------------------------------------------------

  void NodeEditor::moveIceSLWindowAlongChill() {
    if (m_icesl_window != NULL) {
      // get current windows dimensions
      int c_x, c_y, c_w, c_h;
      SDL_GetWindowPosition(m_chill_window, &c_x, &c_y);
      SDL_GetWindowSize(m_chill_window, &c_w, &c_h);

      int i_x, i_y, i_w, i_h;
      SDL_GetWindowPosition(m_chill_window, &i_x, &i_y);
      SDL_GetWindowSize(m_chill_window, &i_w, &i_h);

      if (m_icesl_is_docked) {
        i_x = m_offset_icesl.x * c_w + c_x;
        i_y = m_offset_icesl.y * c_h + c_y;

        i_w = c_w * m_ratio_icesl.x;
        i_h = c_h * m_ratio_icesl.y;
        //raiseIceSL();
      } else {
        i_x = c_w + c_x;
        i_y = c_y;
      }

      SDL_SetWindowPosition(m_icesl_window, i_x, i_y);
      SDL_SetWindowSize(m_icesl_window, i_w, i_h);

    }
  }
 
  //-------------------------------------------------------
  void NodeEditor::SetIceslPath() {
    if (m_iceslPath.empty()) {
#ifdef WIN32
      m_iceslPath = fs::path(std::string(getenv("PROGRAMFILES")) + std::string("/INRIA/IceSL/bin/IceSL-slicer.exe"));
#elif __linux__
      m_iceslPath = getenv("HOME") + std::string("/icesl/bin/IceSL-slicer");
#endif
    }
    if (!fs::exists(m_iceslPath)) {
      std::cerr << "IceSL executable not found. Please specify the location of IceSL's executable." << std::endl;

      const char * modalTitle = "IceSL was not found...";
      const char * modalText = "Icesl was not found on this computer.\n\nIn order for Chill to work properly, it needs access to IceSL.\n\nPlease specify the location of IceSL's executable on your computer.";

#ifdef WIN32
      uint modalFlags = MB_OKCANCEL | MB_DEFBUTTON1 | MB_SYSTEMMODAL | MB_ICONINFORMATION;

      int modal = MessageBox(m_chill_hwnd, modalText, modalTitle, modalFlags);

      if (modal == 1) {
        m_iceslPath = openFileDialog(&OFD_FILTER_ALL);
        std::cerr << "IceSL location specified: " << m_iceslPath << std::endl;
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

  void NodeEditor::raiseIceSL() {
    if (m_icesl_is_docked) {
      if (m_icesl_window != nullptr) {
#ifdef WIN32
        SetWindowPos(m_chill_hwnd, m_icesl_hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#else
        SDL_RaiseWindow(m_icesl_window);
        SDL_SetWindowInputFocus(m_chill_window); // only for X11 :(
#endif
      }
    }
  }

  void NodeEditor::setLayout(int l) {
    if (l < sizeof(layouts) / sizeof(layouts[0]) ) {
      m_offset_icesl = layouts[l].offset_icesl;
      m_ratio_icesl = layouts[l].ratio_icesl;
      m_layout = l;
    }
  }

  void NodeEditor::dock() {
    m_icesl_is_docked = true;
    SDL_SetWindowResizable(m_icesl_window, SDL_FALSE);
    SDL_SetWindowBordered(m_icesl_window, SDL_FALSE);
    setLayout(m_layout);

    moveIceSLWindowAlongChill();
  }

  void NodeEditor::undock() {
    m_icesl_is_docked = false;
    SDL_SetWindowResizable(m_icesl_window, SDL_TRUE);
    SDL_SetWindowBordered(m_icesl_window, SDL_TRUE);

    moveIceSLWindowAlongChill();
  }

  void NodeEditor::maximize()
  {
    // get desktop dimensions
    int desktop_width, desktop_height;
    getDesktopRes(desktop_width, desktop_height);

    // get chill's window border dimensions
    int top, left, bottom, right;
    SDL_GetWindowBordersSize(m_chill_window, &top, &left, &bottom, &right);

    if (m_icesl_is_docked) {
      SDL_SetWindowPosition(m_chill_window, 0, top);
      SDL_SetWindowSize(m_chill_window, desktop_width, desktop_height - top);
      moveIceSLWindowAlongChill();
    } else {
      SDL_SetWindowPosition(m_chill_window, 0, top);
      SDL_SetWindowSize(m_chill_window, desktop_width / 2, desktop_height - top);

      SDL_SetWindowPosition(m_icesl_window, desktop_width / 2, top);
      SDL_SetWindowSize(m_icesl_window, desktop_width / 2, desktop_height - top);

      //SDL_SetWindowInputFocus(m_chill_window); // only for X11 :(
    }
  }
}
