#pragma once

#include <vector>
#include <stack>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <filesystem>

#include <cstdint>

#define NOMINMAX // disable windows min() max() macros
#include "SDL.h"
#include "SDL_syswm.h"
#include "SDL_video.h"

#ifdef WIN32
  
  //#include <windows.h>
  /*
    if the Windows min() and max() macros are needed, 
    use this syntax in code to bypass it:
    (std::min)(a, b);
    (std::max)(a, b);
  */
#endif

#include "UI.h"
#include "Processor.h"
#include "ProcessingGraph.h"


namespace chill {
  typedef unsigned int uint;
  typedef unsigned char uchar;
}


namespace chill {

#ifdef WIN32
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif

  struct Layout {
    char*  Name;
    ImVec2 offset_icesl;
    ImVec2 ratio_icesl;
    bool   resizable;
  };

  class NodeEditor : public UI {

  public:
    const uint default_width = 800;
    const uint default_height = 600;
    const float c_zoom_motion_scale = 0.1f;

    bool m_auto_save = true;
    bool m_auto_export = true;
    bool m_auto_icesl = true;

    fs::path m_iceslPath           = "";
    fs::path m_graphPath           = "";
    fs::path m_iceSLExportPath     = "";
    fs::path m_iceSLTempExportPath = fs::temp_directory_path() / (std::to_string(std::time(nullptr)) + ".lua");

    std::string m_settingsFileName = "/chill-settings.txt";

    ImVec2 m_offset = ImVec2(0, 0);

    bool   m_graph_menu   = false;
    bool   m_node_menu    = false;

    bool   m_dragging     = false;
    bool   m_selecting    = false;
    bool   m_show_grid    = true;

    bool m_minimized         = false;

    void dock();
    void undock();
    void maximize();

    int m_layout = 0;
    Layout layouts[1] = { 
      // {"name", pos, size, resizable },
      {"preview",ImVec2(0.66, 0.66),ImVec2(0.33, 0.33),false},
      //{"side by side",ImVec2(2.0/3.0, 0.05),ImVec2(1.0/3.0, 0.95),false}
      
    };
    void setLayout(int l);
    ImVec2 m_offset_icesl = ImVec2(0.5, 0.5); //offset of icesl window position compare to chill window left up position compared to the windows size
    ImVec2 m_ratio_icesl  = ImVec2(0.5, 0.5); //ratio of icesl window size compare to chill window size
    void raiseIceSL();

    SDL_Window* m_chill_window = nullptr;
    SDL_Window* m_icesl_window = nullptr;

#ifdef WIN32
    HWND m_chill_hwnd = NULL;
    HWND m_icesl_hwnd = NULL;
    PROCESS_INFORMATION m_icesl_p_info;
#else

#endif

    //-------------------------------------------------------

    static NodeEditor* Instance();

    static int launch();

    void setDefaultAppsPos();

    void moveIceSLWindowAlongChill();

    inline void SetIceslPath();

    std::shared_ptr<ProcessingGraph> getCurrentGraph() {
      return m_graphs.top();
    }

    std::shared_ptr<ProcessingGraph> getMainGraph() {
      std::vector<std::shared_ptr<ProcessingGraph>> memory = std::vector<std::shared_ptr<ProcessingGraph>>();
      while (!m_graphs.empty()) {
        memory.push_back(m_graphs.top());
        m_graphs.pop();
      }

      for (auto it = memory.rbegin(); it < memory.rend(); it++) {
        m_graphs.push(*it);
      }

      return memory.back();
    }

    void setMainGraph(std::shared_ptr<ProcessingGraph> _graph) {
      while (!m_graphs.empty()) m_graphs.pop();
      m_graphs.emplace(_graph);
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

      void shortcutsAction();

      void copy();
      void paste();

      void undo();
      void redo();
      void modify();

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
      void exportIceSL(const fs::path* _path);

      void saveSettings();
      void loadSettings();

      void loadGraph(const fs::path* _path, bool _setAsAutoSavePath);

      // Get current desktop size (without taskbar for windows)
      static void getDesktopRes(int& width, int& height);

      //-------------------------------------------------------

      static NodeEditor* s_instance;

      std::stack<std::shared_ptr<chill::ProcessingGraph>> m_graphs;

      std::deque <std::shared_ptr<ProcessingGraph>> m_undo;
      std::deque <std::shared_ptr<ProcessingGraph>> m_redo;

      std::shared_ptr<ProcessorInput>  m_selected_input;
      std::shared_ptr<ProcessorOutput> m_selected_output;

      std::vector<std::shared_ptr<SelectableUI>>     hovered;
      std::vector<std::shared_ptr<SelectableUI>>     selected;
      std::shared_ptr<ProcessingGraph> buffer;

      float m_zoom = 1.0F;
      ImGuiWindow* m_graphWindow = nullptr;

      bool text_editing;
      bool linking;    
      
      bool m_icesl_start_docked = false; // loadSettings uses this to request starting in docked mode
      bool m_icesl_is_docked    = false; // NOTE: only dock/undock are allowed to manipulate this

  };
}
