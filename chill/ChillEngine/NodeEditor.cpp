#include "NodeEditor.h"
#include "Processor.h"
#include "LuaProcessor.h"
#include "IOs.h"
#include "GraphSaver.h"
#include "FileDialog.h"
#include "Resources.h"
#include "VisualComment.h"


#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef USE_GLUT
#include <GL/glut.h>
#endif

LIBSL_WIN32_FIX

bool m_auto_save = true;
bool m_auto_export = true;
bool m_auto_icesl = true;

std::string iceSLExportPath = "";
#ifdef WIN32
std::string iceSLTempExportPath = getenv("TEMP") + std::to_string(std::time(0)) + ".lua";
#endif

const int default_width  = 800;
const int default_height = 600;

#ifdef WIN32
HWND icesl_hwnd = NULL;
DWORD icesl_pid = NULL;
#endif

Chill::NodeEditor *Chill::NodeEditor::s_instance = nullptr;

//-------------------------------------------------------
Chill::NodeEditor::NodeEditor(){
  m_graphs.push(new ProcessingGraph());
}

//-------------------------------------------------------
void Chill::NodeEditor::mainRender()
{
  glClearColor(0.F, 0.F, 0.F, 0.F);
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
  if (sc == LIBSL_KEY_CTRL) {
    ImGui::GetIO().KeyCtrl = true;
  }
}

//-------------------------------------------------------
void Chill::NodeEditor::mainScanCodeUnpressed(uint sc)
{
  if (sc == LIBSL_KEY_SHIFT) {
    ImGui::GetIO().KeyShift = false;
  }
  if (sc == LIBSL_KEY_CTRL) {
    ImGui::GetIO().KeyCtrl = false;
  }
}

//-------------------------------------------------------
void Chill::NodeEditor::mainMouseMoved(uint x, uint y) {}

//-------------------------------------------------------
void Chill::NodeEditor::mainMousePressed(uint x, uint y, uint button, uint flags) {}

void Chill::NodeEditor::getScreenRes(int& width, int& height) {
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
void Chill::NodeEditor::getDesktopScreenRes(int& width, int& height) {
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
#ifdef WIN32
BOOL CALLBACK EnumWindowsFromPid(HWND hwnd, LPARAM lParam)
{
  DWORD lpdwProcessId;
  GetWindowThreadProcessId(hwnd, &lpdwProcessId);
  if (lpdwProcessId == lParam)
  {
    icesl_hwnd = hwnd;
    return FALSE;
  }
  return TRUE;
}
#endif

//-------------------------------------------------------
void Chill::NodeEditor::launch()
{
  int screen_width, screen_height, desktop_width, desktop_height = 0;

  // get screnn / desktop dimmensions
  Chill::NodeEditor::getScreenRes(screen_width, screen_height);
  Chill::NodeEditor::getDesktopScreenRes(desktop_width, desktop_height);

  int app_width = 2 * (desktop_width / 3);
  int app_heigth = desktop_height;

  //int app_pos_x = desktop_width - screen_width;
  //int app_pos_y = desktop_height - screen_height;

  int app_pos_x = - 8; // TODO  PB:get a correct offset / resolution calculation
  int app_pos_y = 0;

  Chill::NodeEditor *nodeEditor = Chill::NodeEditor::Instance();

  try {
    // create window
    SimpleUI::init(default_width, default_height, "Chill, the node-based editor");

    // attach functions
    SimpleUI::onRender             = NodeEditor::mainRender;
    SimpleUI::onKeyPressed         = NodeEditor::mainKeyPressed;
    SimpleUI::onScanCodePressed    = NodeEditor::mainScanCodePressed;
    SimpleUI::onScanCodeUnpressed  = NodeEditor::mainScanCodeUnpressed;
    SimpleUI::onMouseButtonPressed = NodeEditor::mainMousePressed;
    SimpleUI::onMouseMotion        = NodeEditor::mainMouseMoved;
    SimpleUI::onReshape            = NodeEditor::mainOnResize;

    // imgui
    SimpleUI::bindImGui();
    SimpleUI::initImGui();
    SimpleUI::onReshape(default_width, default_height);

    if (m_auto_icesl) {
    // place and dock window to the left
#ifdef WIN32
    // get app handler
    HWND chill_hwnd = SimpleUI::getHWND();
    // move to position
    SetWindowPos(chill_hwnd, HWND_TOPMOST, app_pos_x, app_pos_y, app_width, app_heigth, SWP_SHOWWINDOW);

    // launching Icesl
    


    launchIcesl();
    // move icesl to the last part of the screen
    int icesl_x_offset = 22; // TODO PB:get a correct offset / resolution calculation
    SetWindowPos(icesl_hwnd, HWND_TOP, app_width - icesl_x_offset, app_pos_y, screen_width - app_width + icesl_x_offset, app_heigth, SWP_SHOWWINDOW);

#endif
    }
    // main loop
    SimpleUI::loop();

    // clean up
    SimpleUI::terminateImGui();

    // shutdown UI
    SimpleUI::shutdown();

    if (m_auto_icesl) {
      // closing Icesl
      closeIcesl();
    }
  }
  catch (Fatal& e) {
    std::cerr << Console::red << e.message() << Console::gray << std::endl;
  }
}

//-------------------------------------------------------
void Chill::NodeEditor::launchIcesl() {
#ifdef WIN32
  std::cerr << m_nodeFolder;

  // CreateProcess init
  const char* icesl_path = "C:\\Program Files\\INRIA\\IceSL\\bin\\IceSL-slicer.exe";
  std::string icesl_params = "--remote " + iceSLTempExportPath;

  PROCESS_INFORMATION ProcessInfo;
  STARTUPINFO StartupInfo;
  ZeroMemory(&StartupInfo, sizeof(StartupInfo));
  StartupInfo.cb = sizeof StartupInfo;

  // create the process
  auto icesl_process = CreateProcess(icesl_path, //application name / path
          LPSTR(icesl_params.c_str()), // command line for the application
          NULL, // SECURITY_ATTRIBUTES for the process
          NULL, // SECURITY_ATTRIBUTES for the thread
          FALSE, // inherit handles ?
          CREATE_NEW_CONSOLE, // process creation flags
          NULL, // environment
          NULL, // current directory
          &StartupInfo, // startup info
          &ProcessInfo); // process info

  if(icesl_process)
  {
    // watch the process
    WaitForSingleObject(ProcessInfo.hProcess, 1000);
    // getting the hwnd
    icesl_pid = ProcessInfo.dwProcessId;

    EnumWindows(EnumWindowsFromPid, icesl_pid);
  }
  else
  {
    // process creation failed
    std::cerr << Console::red << "Icesl couldn't be opened, please launch Icesl manually" << Console::gray << std::endl;
    //std::cerr << Console::yellow << GetLastError() << Console::gray << std::endl;

    icesl_hwnd = NULL;
  }
#endif
}

//-------------------------------------------------------
void Chill::NodeEditor::closeIcesl() {
#ifdef WIN32
  // gettings back Icesl's handle
  HANDLE icesl_handle = OpenProcess(PROCESS_ALL_ACCESS, TRUE, icesl_pid);

  // closing the process
  LPDWORD icesl_ThError = NULL, icesl_PrError = NULL;
  TerminateThread(icesl_handle, GetExitCodeThread(icesl_handle, icesl_ThError));
  TerminateProcess(icesl_handle, GetExitCodeProcess(icesl_handle, icesl_PrError));

  // releasing the handles
  CloseHandle(icesl_handle);
  CloseHandle(icesl_handle);
#endif
}

//-------------------------------------------------------
void Chill::NodeEditor::exportIceSL(std::string& filename) {
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
      "------------------------------------------------------" << std::endl;



    getMainGraph()->iceSL(file);
    file.close();
  }
}



//-------------------------------------------------------
bool Chill::NodeEditor::draw()
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

namespace Chill
{
  void zoom();
  void drawGrid();
  void menus();

  void selectProcessors();

  ImVec2 m_offset    = ImVec2(0, 0);

  bool   m_graph_menu = false;
  bool   m_node_menu  = false;

  bool   m_dragging  = false;
  bool   m_selecting = false;
  bool   m_visible   = true;
  bool   m_show_grid = true;

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
        if (ImGui::MenuItem("Save graph", "Ctrl+S")) {
          std::string graph_filename = getMainGraph()->name() + ".graph";
          fullpath = saveFileDialog(graph_filename.c_str(), OFD_FILTER_GRAPHS);
          if (!fullpath.empty()) {
            std::ofstream file;
            file.open(fullpath);
            getMainGraph()->save(file);
            file.close();
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
          iceSLExportPath = saveFileDialog(graph_filename.c_str(), OFD_FILTER_LUA);
          exportIceSL(iceSLExportPath);
        }
        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Settings")) {
        ImGui::MenuItem("Automatic save", "", &m_auto_save);
        ImGui::MenuItem("Automatic export", "", &m_auto_export);
        ImGui::MenuItem("Automatic use of IceSL", "", &m_auto_icesl);
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

    ImGui::Text("Graph name:");
    ImGui::SameLine();
    strcpy(title, m_graphs.top()->name().c_str());
    if (ImGui::InputText(("##" + std::to_string(getUniqueID())).c_str(), title, 16)) {
      m_graphs.top()->setName(title);
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




  bool dirty = true;
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

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImDrawList* overlay_draw_list = ImGui::GetOverlayDrawList();

    ImVec2 w_pos  = window->Pos;
    ImVec2 w_size = window->Size;
    float w_scale = window->FontWindowScale;

    ImVec2 offset   = (m_offset * w_scale + (w_size - w_pos) / 2.0F);

    ImGuiIO io = ImGui::GetIO();

    for (AutoPtr<Processor> processor : m_graphs.top()->processors()) {
      if (processor->isDirty()) {
        dirty = true;
      }
    }

    if (dirty) {
#ifdef WIN32
      exportIceSL(iceSLTempExportPath);
#endif
      exportIceSL(iceSLExportPath);

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


      /*if (selected_processors.empty()) {
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
          if (selected.empty() || (selected.size() == 1 && hovered.empty() )){
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
            if(!com.isNull())
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

      if(io.KeysDown[LIBSL_KEY_CTRL] && io.KeysDown['c'] && io.KeysDownDuration['c'] == 0) {
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
      offset = (m_offset * w_scale + (w_size - w_pos) / 2.0F) ;


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

          ImVec2 bezier( abs(A.x - B.x) / 2.0F * w_scale, 0.0F);

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
      } else if (!m_selected_output.isNull()) {
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
  void selectProcessors() {
    m_selecting = true;

    NodeEditor* n_e = NodeEditor::Instance();
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
  void drawGrid() {
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImVec2 offset = (m_offset * window->FontWindowScale + (window->Size - window->Pos) / 2.F);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const ImU32& GRID_COLOR = style.graph_grid_color;
    const float grid_Line_width = style.graph_grid_line_width;

    int coeff = window->FontWindowScale >= 0.5F ? 1 : 10;
    int subdiv_levels[] = {
      style.graph_grid_size * coeff,
      style.graph_grid_size * coeff * 10};

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
  void zoom() {
    //NodeEditor* n_e = NodeEditor::Instance();
    //ImDrawList* draw_list = ImGui::GetWindowDrawList();
    //ImDrawList* overlay_draw_list = ImGui::GetOverlayDrawList();

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 offset = (m_offset * window->FontWindowScale + (window->Size - window->Pos) / 2.F);
    ImGuiIO io = ImGui::GetIO();


    if (true || io.FontAllowUserScaling) {

      float old_scale = window->FontWindowScale;
      float new_scale = ImClamp(window->FontWindowScale + io.MouseWheel * 0.1F, 0.3F, 2.0F);
      float scale = new_scale / window->FontWindowScale;
      window->FontWindowScale = new_scale;

      const ImVec2 offset = window->Size * (1.F - scale) * (io.MousePos - window->Pos) / window->Size;
      window->Pos += offset;
      window->Pos += offset;
      window->Size *= scale;
      window->SizeFull *= scale;

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
    listFiles(NodeEditor::NodeFolder().c_str(), files);
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
    int nfsize = (int)NodeEditor::NodeFolder().size();
    if (path[nfsize + 1] == Resources::separator()) nfsize++;
    std::string name = path.substr(nfsize);
    return name;
  }

  void addNodeMenu(ImVec2 pos) {
    NodeEditor* n_e = NodeEditor::Instance();
    std::string node = recursiveFileSelecter(NodeEditor::NodeFolder());
    if (!node.empty()) {
      AutoPtr<LuaProcessor> proc = n_e->getCurrentGraph()->addProcessor<LuaProcessor>(relativePath(node));
      proc->setPosition(pos);
    }
  }

  //-------------------------------------------------------
  void menus() {
    NodeEditor* n_e = NodeEditor::Instance();

    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImVec2 w_pos = window->Pos;
    ImVec2 w_size = window->Size;
    float w_scale = window->FontWindowScale;

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
      ImVec2 m2s = io.MousePos - (w_pos + w_size) / 2.0F;
      // screen to grid
      ImVec2 s2g = m2s / w_scale - m_offset;

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
      m_node_menu = false;
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
}
