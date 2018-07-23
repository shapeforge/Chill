#pragma once

#include <vector>
#include <stack>

#include <LibSL/LibSL.h>
#include <LibSL/LibSL_gl.h>

#include "UI.h"
#include "Processor.h"
#include "ProcessingGraph.h"

namespace Chill
{
  class NodeEditor : public UI
  {
  private:
    NodeEditor & operator= (const NodeEditor&) {};
    NodeEditor(const NodeEditor&) {};

    static NodeEditor *s_instance;
    NodeEditor();
    ~NodeEditor(){}

  public:
    static void NodeEditor::launch();

    static NodeEditor *Instance() {
      if (!s_instance)
        s_instance = new NodeEditor;
      return s_instance;
    }

  private:
    std::stack<Chill::ProcessingGraph*> m_graphs;

    AutoPtr<ProcessorInput>  m_selected_input;
    AutoPtr<ProcessorOutput> m_selected_output;

  public:
    /**
     *  Called at each frame
     */
    static void mainRender();

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

  public:
    void exportIceSL(std::string& filename_);

    bool draw();
    void drawMenuBar();
    void drawLeftMenu();
    void drawGraph();

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
      while(! m_graphs.empty()) m_graphs.pop();
      m_graphs.push(_graph);
    }

    void setSelectedInput(AutoPtr<ProcessorInput> _input) {
      m_selected_input = _input;

      if (! m_selected_output.isNull()) {
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