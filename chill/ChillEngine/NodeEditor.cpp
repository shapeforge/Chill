#include "NodeEditor.h"
#include "Processor.h"
#include "LuaProcessor.h"
#include "IOs.h"
#include "GraphSaver.h"
#include "FileDialog.h"
#include "Resources.h"

#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef USE_GLUT
#include <GL/glut.h>
#endif

LIBSL_WIN32_FIX

const int default_width  = 800;
const int default_height = 600;

std::string nodeFolder = "";

Chill::NodeEditor *Chill::NodeEditor::s_instance = nullptr;

//-------------------------------------------------------
Chill::NodeEditor::NodeEditor(){
  m_graphs.push(new ProcessingGraph());
}

//-------------------------------------------------------
void Chill::NodeEditor::mainRender()
{
  glClearColor(0.f, 0.f, 0.f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  Instance()->draw();

  ImGui::Render();
}

//-------------------------------------------------------
void Chill::NodeEditor::mainOnResize(uint width, uint height) {
  Instance()->m_size = ImVec2((float)width, (float)height);
}

//-------------------------------------------------------
void Chill::NodeEditor::mainKeyPressed(uchar k)
{
}

//-------------------------------------------------------
void Chill::NodeEditor::mainScanCodePressed(uint sc)
{
  if (sc == LIBSL_KEY_SHIFT) {
    ImGui::GetIO().KeyShift = true;
  }
}

//-------------------------------------------------------
void Chill::NodeEditor::mainScanCodeUnpressed(uint sc)
{
  if (sc == LIBSL_KEY_SHIFT) {
    ImGui::GetIO().KeyShift = false;
  }
}

//-------------------------------------------------------
void Chill::NodeEditor::mainMouseMoved(uint x, uint y)
{
}

//-------------------------------------------------------
void Chill::NodeEditor::mainMousePressed(uint x, uint y, uint button, uint flags)
{
}

//-------------------------------------------------------
void Chill::NodeEditor::launch()
{
  Chill::NodeEditor *nodeEditor = Chill::NodeEditor::Instance();
  try {
    // create window
    SimpleUI::init(default_width, default_height, "Chill, the node-based editor");

    // attach functions
    SimpleUI::onRender = nodeEditor->mainRender;
    SimpleUI::onKeyPressed = nodeEditor->mainKeyPressed;
    SimpleUI::onScanCodePressed = nodeEditor->mainScanCodePressed;
    SimpleUI::onScanCodeUnpressed = nodeEditor->mainScanCodeUnpressed;
    SimpleUI::onMouseButtonPressed = nodeEditor->mainMousePressed;
    SimpleUI::onMouseMotion = nodeEditor->mainMouseMoved;
    SimpleUI::onReshape = nodeEditor->mainOnResize;

    // imgui
    SimpleUI::bindImGui();
    SimpleUI::initImGui();
    SimpleUI::onReshape(default_width, default_height);

    // main loop
    SimpleUI::loop();

    // clean up
    SimpleUI::terminateImGui();

    // shutdown UI
    SimpleUI::shutdown();
  }
  catch (Fatal& e) {
    std::cerr << Console::red << e.message() << Console::gray << std::endl;
  }
}




void Chill::NodeEditor::exportIceSL(std::string& filename) {
  if (!filename.empty()) {
    std::ofstream file;
    file.open(filename);

    // TODO: CLEAN THIS !!!

    file << "\
enable_variable_cache = true\n\
\n\
local _G0 = {}       --swap environnement(swap variables between scripts)\n\
local _Gcurrent = {} --environment local to the script : _Gc includes _G0\n\
local __dirty = {}   --table of all dirty nodes\n\
__input = {}         --table of all input values\n\
\n\
setmetatable(_G0, { __index = _G })\n\
\n\
function setNodeId(id)\n\
  setfenv(1, _G0)\n\
  __currentNodeId = id\n\
  setfenv(1, _Gcurrent)\n\
end\n\
\n\
function input(name, type, ...)\n\
  return __input[name]\n\
end\n\
\n\
function output(name, type, val)\n\
  setfenv(1, _G0)\n\
  if (isDirty({ __currentNodeId })) then\n\
    _G[name..__currentNodeId] = val\n\
  end\n\
  setfenv(1, _Gcurrent)\n\
end\n\
\n\
function setDirty(node)\n\
  __dirty[node] = true\n\
end\n\
\n\
function isDirty(nodes)\n\
  if first_exec then\n\
    return true\n\
  end\n\
\n\
  if #nodes == 0 then\n\
    return false\n\
  else\n\
    local node = table.remove(nodes, 1)\n\
    if node == NIL then node = nil end\n\
    return __dirty[node] or isDirty(nodes)\n\
  end\n\
end\n\
\n\
if first_exec == nil then\n\
  first_exec = true\n\
else\n\
  first_exec = false\n\
end\n\
\n\
------------------------------------------------------\n\
";



    getMainGraph()->iceSL(file);
    file.close();
  }
}



//-------------------------------------------------------
bool Chill::NodeEditor::draw()
{
  drawMenuBar();

  ImGui::SetNextWindowPos(ImVec2(200, 20));
  ImGui::SetNextWindowSize(m_size);

  drawGraph();

  return true;
}

namespace Chill
{
  void zoom();
  void drawGrid();
  void menus();

  void selectProcessors();

  ImVec2 m_offset    = ImVec2(0, 0);

  ImVec2 win_pos;
  ImVec2 win_size;

  bool   m_graph_menu = false;
  bool   m_node_menu  = false;

  bool   m_dragging  = false;
  bool   m_selecting = false;
  bool   m_visible   = true;
  bool   m_show_grid = true;

  std::string iceSlExportPath = "";

  Style style;

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
          }
        }
        if (ImGui::MenuItem("Load graph")) {
          fullpath = openFileDialog(OFD_FILTER_GRAPHS);
          if (!fullpath.empty()) {
            GraphSaver *test = new GraphSaver();
            test->execute(fullpath.c_str());
            delete test;
          }
        }
        if (ImGui::MenuItem("Save graph")) {
          std::string graph_filename = getMainGraph()->name() + ".graph";
          fullpath = saveFileDialog(graph_filename.c_str(), OFD_FILTER_GRAPHS);
          if (!fullpath.empty()) {
            std::ofstream file;
            file.open(fullpath);
            getMainGraph()->save(file);
            file.close();
          }
        }
        if (ImGui::MenuItem("Save current graph")) {
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
            nodeFolder = fullpath.substr(0, last_slash_idx) + Resources::separator() + "nodes";
          }
        }


        if (ImGui::MenuItem("Export to IceSL lua")) {
          std::string graph_filename = getMainGraph()->name() + ".lua";
          iceSlExportPath = saveFileDialog(graph_filename.c_str(), OFD_FILTER_LUA);
          exportIceSL(iceSlExportPath);
        }
        ImGui::EndMenu();
      }

      bool test = true;
      if (ImGui::BeginMenu("Parameters")) {
        ImGui::MenuItem("Automatic save", "", test);
        ImGui::MenuItem("Automatic export", "", test);
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
  }


  std::vector<AutoPtr<Processor>> hovered_processors;
  std::vector<AutoPtr<Processor>> selected_processors;
  AutoPtr<ProcessingGraph> buffer;

  bool dirty;
  bool text_editing;
  bool linking;

  //-------------------------------------------------------
  void NodeEditor::drawGraph()
  {
    linking = !m_selected_input.isNull() || !m_selected_output.isNull();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, style.graph_bg_color);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("Graph", &m_visible,
      ImGuiWindowFlags_NoTitleBar  |
      ImGuiWindowFlags_NoCollapse  |

      ImGuiWindowFlags_NoMove            |
      ImGuiWindowFlags_NoResize          |
      ImGuiWindowFlags_NoScrollbar       |
      ImGuiWindowFlags_NoScrollWithMouse |
      
      ImGuiWindowFlags_NoBringToFrontOnFocus
    );

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImDrawList* overlay_draw_list = ImGui::GetOverlayDrawList();

    win_pos  = ImGui::GetCursorScreenPos();
    win_size = ImGui::GetWindowSize();

    ImVec2 offset   = (m_offset * m_scale + (win_size - win_pos) / 2.0f);

    ImGuiIO io = ImGui::GetIO();

    for (AutoPtr<Processor> processor : m_graphs.top()->processors()) {
      if (processor->isDirty()) {
        dirty = true;
      }
    }

    if (dirty) {
      exportIceSL(iceSlExportPath);
      dirty = false;
    }

    if (ImGui::IsWindowHovered()) {
      hovered_processors.clear();
      selected_processors.clear();
      
      text_editing = false;

      for (AutoPtr<Processor> processor : m_graphs.top()->processors()) {
        ImVec2 socket_size = ImVec2(1, 1) * (style.socket_radius + style.socket_border_width) * m_scale;

        ImVec2 size = processor->m_size * processor->m_scale;
        ImVec2 min_pos = offset + win_pos + processor->m_position * m_scale - socket_size;
        ImVec2 max_pos = min_pos + size + socket_size * 2;

        if (ImGui::IsMouseHoveringRect(min_pos, max_pos)) {
          hovered_processors.push_back(processor);
        }

        if (processor->m_selected) {
          selected_processors.push_back(processor);
        }

        if (processor->m_edit) {
          text_editing = true;
        }
      }

      // LEFT CLICK
      if (linking) {
        m_selecting = false;
        m_dragging = false;

        if (io.MouseDown[0] && hovered_processors.empty() || io.MouseDown[1]) {
          linking = false;
          m_selected_input = AutoPtr<ProcessorInput>(NULL);
          m_selected_output = AutoPtr<ProcessorOutput>(NULL);
        }
      }
      else {
        if (io.MouseDoubleClicked[0]) {
          for (AutoPtr<Processor> hovproc : hovered_processors) {
            if (ProcessingGraph* v = dynamic_cast<ProcessingGraph*>(hovproc.raw())) {
              if (!v->m_edit) {
                m_graphs.push(v);
                m_offset = m_graphs.top()->getBarycenter() * -1.0f;
                break;
              }
            }
          }
        }

        if (io.MouseDown[0] && io.MouseDownOwned[0]) {
          if (!hovered_processors.empty() && !m_selecting) {
            for (AutoPtr<Processor> selproc : selected_processors) {
              for (AutoPtr<Processor> hovproc : hovered_processors) {
                if (hovproc == selproc) {
                  m_dragging = true;
                }
              }
            }
          }

          if (hovered_processors.empty() && !m_dragging || m_selecting) {
            selectProcessors();
          }
        }
        else {
          m_selecting = false;
          m_dragging = false;
        }

        if (io.MouseClicked[0]) {
          // if nothing hovered, clear
          if (!io.KeysDown[LIBSL_KEY_SHIFT] && hovered_processors.empty()) {
            for (AutoPtr<Processor> selproc : selected_processors) {
              selproc->m_selected = false;
            }
            selected_processors.clear();
          }
          else {
            for (AutoPtr<Processor> hovproc : hovered_processors) {
              if (io.KeysDown[LIBSL_KEY_SHIFT]) {
                if (hovproc->m_selected) {
                  hovproc->m_selected = false;
                  selected_processors.erase(std::find(selected_processors.begin(), selected_processors.end(), hovproc));
                }
                else {
                  hovproc->m_selected = true;
                  selected_processors.push_back(hovproc);
                }
              }
              else {
                bool sel_and_hov = false;
                for (AutoPtr<Processor> selproc : selected_processors) {
                  if (selproc == hovproc) {
                    sel_and_hov = true;
                    break;
                  }
                }
                if (!sel_and_hov) {
                  for (AutoPtr<Processor> selproc : selected_processors) {
                    selproc->m_selected = false;
                  }
                  selected_processors.clear();

                  hovproc->m_selected = true;
                  selected_processors.push_back(hovproc);
                }
              }
            }
          }
        }
      } // ! LEFT CLICK



      { // RIGHT CLICK
        if (io.MouseClicked[1]) {
          if (selected_processors.empty()) {
            m_graph_menu = true;
          }
          else {
            m_node_menu = true;
          }
        }
      } // ! RIGHT CLICK



      // Move selected processors
      if (m_dragging) { 
        for (AutoPtr<Processor> selected : selected_processors) {
          selected->translate(io.MouseDelta / m_scale);
        }
      }

      // Move the whole canvas
      if (io.MouseDown[2] && ImGui::IsMouseHoveringAnyWindow()) {
        m_offset += io.MouseDelta / m_scale;
      }

      zoom();


      // Keyboard
      if (!text_editing)
      {
        if (io.KeysDown['g'] && io.KeysDownDuration['g'] == 0) {
          if (!selected_processors.empty()) {
            getCurrentGraph()->collapseSubset(selected_processors);
            selected_processors.clear();
          }
        }

        if (io.KeysDown['b'] && io.KeysDownDuration['b'] == 0) {
          if (m_graphs.size() > 1) {
            m_graphs.pop();
            m_offset = getCurrentGraph()->getBarycenter() * -1.0f;
          }
        }
      }

      if(io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['c'] && io.KeysDownDuration['c'] == 0) {
        if (!selected_processors.empty()) {
          buffer = getCurrentGraph()->copySubset(selected_processors);
        }
      }
      if (!buffer.isNull()) {
        if (ImGui::MenuItem("Paste", "CTRL+V")) {
          //getCurrentGraph()->expandGraph(buffer, s2g);
          buffer = AutoPtr<ProcessingGraph>(NULL);
        }
      }
      if (io.KeysDown[LIBSL_KEY_DELETE]) {
        for (AutoPtr<Processor> proc : selected_processors) {
          getCurrentGraph()->removeProcessor(proc);
        }
      }

      // update the offset used for display
      offset = (m_offset * m_scale + (win_size - win_pos) / 2.0f) ;
      

    } // ! if window hovered

    // Draw the grid
    if (m_show_grid)
    {
      drawGrid();
    }


    ProcessingGraph* currentGraph = m_graphs.top();
    // Draw the nodes
    for (AutoPtr<Processor> processor : currentGraph->processors()) {
      ImVec2 position = offset + processor->m_position * m_scale;
      ImGui::SetCursorPos(position);
      processor->setScale(m_scale);
      processor->draw();
    }

    // Draw the pipes
    float pipe_width = 2.0f * m_scale;
    int pipe_res = int(25.0f / std::abs(std::log(m_scale / 2.0f)));
    for (AutoPtr<Processor> processor : currentGraph->processors()) {
      for (AutoPtr<ProcessorOutput> output : processor->outputs()) {
        for (AutoPtr<ProcessorInput> input : output->m_links) {
          ImVec2 A = input->getPosition() + win_pos;
          ImVec2 B = output->getPosition() + win_pos;

          ImVec2 bezier( abs(A.x - B.x) / 2.0f * m_scale, 0.0f);

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
      float pipe_width = 2.0f * m_scale;
      int pipe_res = int(25.0f / std::abs(std::log(m_scale / 2.0f)));

      ImVec2 A = ImGui::GetMousePos();
      ImVec2 B = ImGui::GetMousePos();

      if (!m_selected_input.isNull()) {
        A = win_pos + m_selected_input->getPosition();
      } else if (!m_selected_output.isNull()) {
        B = win_pos + m_selected_output->getPosition();
      }
      
      ImVec2 bezier(100.0f * m_scale, 0);
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
    ImGui::SetCursorPos(win_pos);
    uint i = 1;

    ImVec2 pos = win_pos;
    ImVec2 size(100, 20);


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

      ImGui::SetCursorPos(pos - win_pos + style.delta_to_center_text(ImVec2(10, size.y), ">"));
      ImGui::Text(">");
      ImGui::SetCursorPos(pos - win_pos + style.delta_to_center_text(size, graph->name().c_str()));
      ImGui::Text(graph->name().c_str());

      ImVec2 title_size = ImGui::CalcTextSize(graph->name().c_str());


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
  void selectProcessors() {
    m_selecting = true;
    
    NodeEditor* n_e = NodeEditor::Instance();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImDrawList* overlay_draw_list = ImGui::GetOverlayDrawList();

    ImVec2 offset = (m_offset * n_e->getScale() + (win_size - win_pos) / 2.0f);

    ImGuiIO io = ImGui::GetIO();


    ImVec2 A = (io.MousePos - (win_pos + win_size) / 2.0f) / n_e->getScale() - m_offset;
    ImVec2 B = (io.MouseClickedPos[0] - (win_pos + win_size) / 2.0f) / n_e->getScale() - m_offset;

    if (A.x > B.x) std::swap(A.x, B.x);
    if (A.y > B.y) std::swap(A.y, B.y);

    if (B.x - A.x > 10 || B.y - A.y > 10) {
      draw_list->AddRect(
        io.MouseClickedPos[0],
        io.MousePos,
        style.processor_selected_color
      );

      if (!io.KeysDown[LIBSL_KEY_SHIFT]) {
        for (AutoPtr<Processor> selproc : selected_processors) {
          selproc->m_selected = false;
        }
        selected_processors.clear();
      }

      for (AutoPtr<Processor> procui : n_e->getCurrentGraph()->processors()) {
        ImVec2 pos_min = procui->getPosition();
        ImVec2 pos_max = pos_min + procui->m_size;

        if (pos_min.x < A.x || B.x < pos_max.x) continue;
        if (pos_min.y < A.y || B.y < pos_max.y) continue;
        procui->m_selected = true;
        selected_processors.push_back(procui);
      }
    }
  }

  //-------------------------------------------------------
  void drawGrid() {
    NodeEditor* n_e = NodeEditor::Instance();

    ImVec2 offset = (m_offset * n_e->getScale() + (win_size - win_pos) / 2.0f);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const ImU32& GRID_COLOR = style.graph_grid_color;
    const float grid_Line_width = style.graph_grid_line_width;

    int coeff = n_e->getScale() >= 0.5f ? 1 : 10;
    int subdiv_levels[] = {
      style.graph_grid_size * coeff,
      style.graph_grid_size * coeff * 10};

    for (int subdiv : subdiv_levels) {
      const float& grid_size = n_e->getScale() * subdiv;

      // Vertical lines
      for (float x = fmodf(offset.x, grid_size); x < win_size.x; x += grid_size) {
        ImVec2 p1 = ImVec2(x + win_pos.x, win_pos.y);
        ImVec2 p2 = ImVec2(x + win_pos.x, win_size.y + win_pos.y);
        draw_list->AddLine(p1, p2, GRID_COLOR, grid_Line_width);
      }

      // Horizontal lines
      for (float y = fmodf(offset.y, grid_size); y < n_e->m_size.y; y += grid_size) {
        ImVec2 p1 = ImVec2(win_pos.x, y + win_pos.y);
        ImVec2 p2 = ImVec2(win_size.x + win_pos.x, y + win_pos.y);
        draw_list->AddLine(p1, p2, GRID_COLOR, grid_Line_width);
      }
    }
  }

  //-------------------------------------------------------
  void zoom() {
    NodeEditor* n_e = NodeEditor::Instance();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImDrawList* overlay_draw_list = ImGui::GetOverlayDrawList();

    ImVec2 offset = (m_offset * n_e->getScale() + (win_size - win_pos) / 2.0f);

    ImGuiIO io = ImGui::GetIO();

    float scale = n_e->getScale();

    if (io.MouseWheel != 0.0f) {
      float new_scale = scale;
      if (io.MouseWheel > 0) {
        new_scale *= 1.1f;
      }
      else {
        new_scale /= 1.1f;
      }
      new_scale = std::min(1.0f, std::max(0.3f, new_scale));

      if (new_scale != scale) {
        // mouse to screen
        ImVec2 m2s = io.MousePos - (win_pos + win_size) / 2.0f;
        // screen to grid
        ImVec2 s2g = m2s / scale - m_offset;
        n_e->setScale(new_scale);
        // grid to screen
        ImVec2 g2s = (s2g + m_offset) * new_scale;

        m_offset += (m2s - g2s) / new_scale;
      }
    }
  }

  //-------------------------------------------------------
  void listFolderinDir(std::vector<std::string>& files, std::string folder)
  {
    using namespace std::experimental::filesystem;

    for (directory_iterator itr(folder); itr != directory_iterator(); ++itr)
    {
      path file = itr->path();
      if (is_directory(file))files.push_back(file.filename().generic_string());
    }
  }

  void listLuaFileInDir(std::vector<std::string>& files)
  {
    listFiles(nodeFolder.c_str(), files);
  }

  void listLuaFileInDir(std::vector<std::string>& files, std::string directory)
  {
    using namespace std::experimental::filesystem;

    for (directory_iterator itr(directory); itr != directory_iterator(); ++itr)
    {
      std::experimental::filesystem::path file = itr->path();
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
      if (ImGui::MenuItem(files[i].c_str())) {
        nameDir = Resources::toPath(current_dir, files[i]);
      }
    }
    ImGui::PopStyleColor();

    return nameDir;
  }

  //-------------------------------------------------------
  std::string relativePath(std::string& path)
  {
    int nfsize = (int)nodeFolder.size();
    if (path[nfsize + 1] == Resources::separator()) nfsize++;
    std::string name = path.substr(nfsize);
    return name;
  }

  void addNodeMenu(ImVec2 pos) {
    NodeEditor* n_e = NodeEditor::Instance();
    std::string node = recursiveFileSelecter(nodeFolder);
    if (!node.empty()) {
      AutoPtr<LuaProcessor> proc = n_e->getCurrentGraph()->addProcessor<LuaProcessor>("nodes" + relativePath(node));
      proc->setPosition(pos);
    }
  }

  //-------------------------------------------------------
  void menus() {
    NodeEditor* n_e = NodeEditor::Instance();
    ImGuiIO io = ImGui::GetIO();

    // Draw menus
    if (m_graph_menu) {
      ImGui::OpenPopup("graph_menu");
    }
    if (m_node_menu) {
      ImGui::OpenPopup("node_menu");
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
    if (ImGui::BeginPopup("graph_menu"))
    {
      // mouse to screen
      ImVec2 m2s = io.MousePos - (win_pos + win_size) / 2.0f;
      // screen to grid
      ImVec2 s2g = m2s / n_e->getScale() - m_offset;

      m_graph_menu = false;
      
      if( ImGui::BeginMenu("Add", "SPACE")) {
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
      if (ImGui::MenuItem("Nothing")) {
        AutoPtr<Processor> proc = n_e->getCurrentGraph()->addProcessor<Processor>();
        proc->addInput(ProcessorInput::create("color", IOType::VEC4, 0, 0, 255, true));
        proc->setPosition(s2g);
      }
      ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("node_menu"))
    {
      m_node_menu = false;
      if (ImGui::MenuItem("Copy", "CTRL+C")) {
        if (!selected_processors.empty()) {
          buffer = n_e->getCurrentGraph()->copySubset(selected_processors);
        }
      }
      if (ImGui::MenuItem("Group")) {
        if (!selected_processors.empty()) {
          n_e->getCurrentGraph()->collapseSubset(selected_processors);
        }
      }
      if (ImGui::MenuItem("Ungroup")) {
        if (!selected_processors.empty()) {
          for (AutoPtr<Processor> proc : selected_processors) {
            if (typeid(*proc.raw()) == typeid(ProcessingGraph)) {
              n_e->getCurrentGraph()->expandGraph(AutoPtr<ProcessingGraph>(proc), proc->getPosition());
            }
          }
        }
      }

      if (ImGui::MenuItem("Delete")) {
        for (AutoPtr<Processor> proc : selected_processors) {
          n_e->getCurrentGraph()->removeProcessor(proc);
        }
      }
      if (ImGui::MenuItem("Unlink")) {
        for (AutoPtr<Processor> proc : selected_processors) {
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
      if (ImGui::MenuItem("Nothing")) {}
      ImGui::EndPopup();
    }
    ImGui::PopStyleVar();
  }
}