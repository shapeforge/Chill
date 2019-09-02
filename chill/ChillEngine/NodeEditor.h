#pragma once

#include <vector>
#include <stack>

#include <iostream>
#include <fstream>
#include <filesystem>

#include <LibSL/LibSL.h>
#include <LibSL/LibSL_gl.h>

#include "UI.h"
#include "Processor.h"
#include "ProcessingGraph.h"

namespace fs = std::experimental::filesystem;

namespace Chill
{
  class NodeEditor : public UI
  {
  private:
    static NodeEditor* s_instance;

    std::stack<Chill::ProcessingGraph*> m_graphs;

    AutoPtr<ProcessorInput>  m_selected_input;
    AutoPtr<ProcessorOutput> m_selected_output;

    std::vector<AutoPtr<SelectableUI>>     hovered;
    std::vector<AutoPtr<SelectableUI>>     selected;
    AutoPtr<ProcessingGraph> buffer;

    bool text_editing;
    bool linking;

    //-------------------------------------------------------

    NodeEditor();
    ~NodeEditor() {}

    NodeEditor & operator= (const NodeEditor&) {};
    NodeEditor(const NodeEditor&) {};

    /**
     *  Called at each frame
     */
    static void mainRender();

    static void setIcon();

    /**
     *  Called on resize
     *  @param width the new width
     *  @param height the new height
     */
    static void mainOnResize(uint _width, uint _height);

    /**
     *  Called when a key is pressed
     *  @param k the typed character
     */
    static void mainKeyPressed(uchar _k);
    static void mainScanCodePressed(uint _sc);
    static void mainScanCodeUnpressed(uint _sc);
    static void mainMouseMoved(uint _x, uint _y);
    static void mainMousePressed(uint _x, uint _y, uint _button, uint _flags);

    bool draw();
    void drawMenuBar();
    void drawLeftMenu();
    void drawGraph();

    void zoom();
    void drawGrid();
    void menus();

    void selectProcessors();

    static void launchIcesl();
    static void closeIcesl();

    // export the graph to a .lua file for IceSL
    void exportIceSL(std::string& filename_);

    void saveSettings();
    void loadSettings();

    // Get current screen size
    static void Chill::NodeEditor::getScreenRes(int& width, int& height);
    // Get current desktop size (without taskbar for windows)
    static void Chill::NodeEditor::getDesktopScreenRes(int& width, int& height);

  public:
    const int default_width = 800;
    const int default_height = 600;

    bool g_auto_save = true;
    bool g_auto_export = true;
    bool g_auto_icesl = true;

    std::string g_iceslPath;

    std::string g_graphPath = "";
    std::string g_iceSLExportPath = "";
    std::string g_iceSLTempExportPath = fs::temp_directory_path().string() + std::to_string(std::time(0)) + ".lua";

    std::string g_settingsFileName = "chill-settings.txt";

    ImVec2 m_offset = ImVec2(0, 0);

    bool   m_graph_menu = false;
    bool   m_node_menu = false;

    bool   m_dragging = false;
    bool   m_selecting = false;
    bool   m_visible = true;
    bool   m_show_grid = true;

    Style style;

#ifdef WIN32
    HWND g_chill_hwnd = NULL;
    HWND g_icesl_hwnd = NULL;
    PROCESS_INFORMATION g_icesl_p_info;
#endif

    //-------------------------------------------------------

    static NodeEditor* Instance() {
      if (!s_instance)
        s_instance = new NodeEditor;
      return s_instance;
    }

    static void Chill::NodeEditor::launch();  

    void setDefaultAppsPos();
    void moveIceSLWindowAlongChill();

    static inline std::string ChillFolder();
    static inline std::string NodesFolder();
    inline void SetIceslPath();

    ProcessingGraph* getCurrentGraph() {
      return m_graphs.top();
    }

    ProcessingGraph* getMainGraph() {
      std::vector<ProcessingGraph*> memory = std::vector<ProcessingGraph*>();
      while (!m_graphs.empty()) {
        memory.push_back(m_graphs.top());
        m_graphs.pop();
      }

      for (auto it = memory.rbegin(); it < memory.rend(); it++) {
        m_graphs.push(*it);
      }

      return memory.back();
    }

    void setMainGraph(ProcessingGraph* _graph) {
      while (!m_graphs.empty()) m_graphs.pop();
      m_graphs.push(_graph);
    }

    void setSelectedInput(AutoPtr<ProcessorInput> _input) {
      m_selected_input = _input;

      if (!m_selected_output.isNull()) {
        if (m_selected_output->owner() == m_selected_input->owner()) {
          return;
        }
        getCurrentGraph()->connect(m_selected_output, _input);
        m_selected_input = AutoPtr<ProcessorInput>(NULL);
        m_selected_output = AutoPtr<ProcessorOutput>(NULL);
      }
    }

    AutoPtr<ProcessorInput> getSelectedInput() {
      return m_selected_input;
    }

    void setSelectedOutput(AutoPtr<ProcessorOutput> _output) {
      m_selected_output = _output;

      if (!m_selected_input.isNull()) {
        if (m_selected_output->owner() == m_selected_input->owner()) {
          return;
        }
        getCurrentGraph()->connect(_output, m_selected_input);
        m_selected_input = AutoPtr<ProcessorInput>(NULL);
        m_selected_output = AutoPtr<ProcessorOutput>(NULL);
      }
    }

    AutoPtr<ProcessorOutput> getSelectedOutput() {
      return m_selected_output;
    }

  };
};
