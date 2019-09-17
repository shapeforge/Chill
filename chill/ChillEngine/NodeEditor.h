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

#ifdef WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

namespace chill {
  class NodeEditor : public UI {


  public:
    const uint default_width = 800;
    const uint default_height = 600;
    const float c_zoom_motion_scale = 0.1f;

    bool m_auto_save = true;
    bool m_auto_export = true;
    bool m_auto_icesl = true;

    std::string m_iceslPath;

    std::string m_graphPath = "";
    std::string m_iceSLExportPath = "";
    fs::path m_iceSLTempExportPath = fs::temp_directory_path() / (std::to_string(std::time(nullptr)) + ".lua");

    std::string m_settingsFileName = "/chill-settings.txt";

    ImVec2 m_offset = ImVec2(0, 0);

    bool   m_graph_menu = false;
    bool   m_node_menu = false;

    bool   m_dragging = false;
    bool   m_selecting = false;
    bool   m_show_grid = true;

#ifdef WIN32
    HWND m_chill_hwnd = NULL;
    HWND m_icesl_hwnd = NULL;
    PROCESS_INFORMATION m_icesl_p_info;
#endif

    //-------------------------------------------------------

    static NodeEditor* Instance();

    static void launch();

#ifdef WIN32
    void setDefaultAppsPos(HMONITOR hMonitor);
#else
    void setDefaultAppsPos();
#endif   

    void resizeIceSLWindowAlongChill();
    void moveIceSLWindowAlongChill();

    static inline std::string ChillFolder();
    static inline std::string NodesFolder();
    static bool scriptPath(const std::string& name, std::string& _path);
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

    void setSelectedInput(std::shared_ptr<ProcessorInput> _input) {
      m_selected_input = _input;

      if (m_selected_output) {
        if (m_selected_output->owner() == m_selected_input->owner()) {
          return;
        }
        getCurrentGraph()->connect(m_selected_output, _input);
        m_selected_input = std::shared_ptr<ProcessorInput>(nullptr);
        m_selected_output = std::shared_ptr<ProcessorOutput>(nullptr);
      }
    }

    std::shared_ptr<ProcessorInput> getSelectedInput() {
      return m_selected_input;
    }

    void setSelectedOutput(std::shared_ptr<ProcessorOutput> _output) {
      m_selected_output = _output;

      if (m_selected_input) {
        if (m_selected_output->owner() == m_selected_input->owner()) {
          return;
        }
        getCurrentGraph()->connect(_output, m_selected_input);
        m_selected_input = std::shared_ptr<ProcessorInput>(nullptr);
        m_selected_output = std::shared_ptr<ProcessorOutput>(nullptr);
      }
    }

    std::shared_ptr<ProcessorOutput> getSelectedOutput() {
      return m_selected_output;
    }


    private:
      NodeEditor();
      ~NodeEditor() = default;

      NodeEditor & operator= (const NodeEditor&) = delete;
      NodeEditor(const NodeEditor&)              = delete;

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
      void exportIceSL(const fs::path filename_);

      void saveSettings();
      void loadSettings();

      // Get current screen size
      static void getScreenRes(int& width, int& height);
      // Get current desktop size (without taskbar for windows)
      static void getDesktopRes(int& width, int& height);

      //-------------------------------------------------------

      static NodeEditor* s_instance;

      std::stack<chill::ProcessingGraph*> m_graphs;

      std::shared_ptr<ProcessorInput>  m_selected_input;
      std::shared_ptr<ProcessorOutput> m_selected_output;

      std::vector<std::shared_ptr<SelectableUI>>     hovered;
      std::vector<std::shared_ptr<SelectableUI>>     selected;
      std::shared_ptr<ProcessingGraph> buffer;

      bool text_editing;
      bool linking;

  };
}
