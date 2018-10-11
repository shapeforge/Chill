/** @file */
#pragma once

// Processor.h
namespace Chill
{
  class Processor;
};

namespace Chill
{
  class ProcessingGraph;
};

#include <vector>
#include <set>
#include <unordered_set>
#include <LibSL.h>

#include "Processor.h"



namespace Chill
{
  /**
   * Graph class. Contains all nodes and connections.
   */
  class ProcessingGraph : public Processor
  {
  private:
    // inner_input -> visible_output
    typedef std::pair<AutoPtr<ProcessorInput>, AutoPtr<ProcessorOutput>> GroupInput;
    // inner_output -> visible_input
    typedef std::pair<AutoPtr<ProcessorOutput>, AutoPtr<ProcessorInput>> GroupOutput;

  protected:
    /** List of all processors within this graph. */
    std::vector<AutoPtr<Processor>> m_processors;
    /** List of all entry points */
    std::vector<GroupInput>         m_group_inputs;
    /** List of all exit points */
    std::vector<GroupOutput>        m_group_outputs;

  private:
    ProcessingGraph(ProcessingGraph &_copy);

  public:

    ProcessingGraph() {
      setName ("graph");
      setColor(style.processor_graph_color);
    }

    ProcessingGraph(const std::string& _name) {
      setName (_name);
      setColor(style.processor_graph_color);
    }

    ~ProcessingGraph();

    /**
     *  Add a new processor to the graph.
     *  @return The AutoPtr related to the processor created.
     **/
    template<typename T_Processor, typename... Args>
    AutoPtr<T_Processor> addProcessor(Args&& ... args)
    {
      AutoPtr<T_Processor> processor(new T_Processor(args...));
      processor->setOwner(this);
      m_processors.push_back((AutoPtr<Processor>)processor);
      return processor;
    }

    /**
     *  Add an existing processor to the graph.
     *  @param _processor The AutoPtr related to the processor.
     **/
    void addProcessor(AutoPtr<Processor>& _processor)
    {
      sl_assert(&_processor);
      sl_assert(_processor->owner() != this);

      _processor->setOwner(this);
      m_processors.push_back(_processor);
    }

    /**
     *  Remove an existing processor from the graph.
     *  @param _processor The AutoPtr related to the processor.
     **/
    void removeProcessor(AutoPtr<Processor>& _processor);

    /**
     *  Add a new connection to the graph if, and only if, there is no cycle created.
     *  @param _from The origin processor.
     *  @param _output_name The name of the output.
     *  @param _to The destination processor.
     *  @param _input_name The name of the input.
     **/
    static bool connect(const AutoPtr<Processor>& _from, const std::string _output_name, const AutoPtr<Processor>& _to, const std::string _input_name);
    
    /**
     *  Add a new connection to the graph if, and only if, there is no cycle created.
     *  @param _from The processor's output.
     *  @param _to The processor's input.
     **/
    static bool connect(AutoPtr<ProcessorOutput>& _from, AutoPtr<ProcessorInput>& _to);

    /**
     *  Remove a connection in the graph.
     *  @param _to The processor's input.
     **/
    static void disconnect(AutoPtr<ProcessorInput>& _to);

    /**
    *  Remove a connection in the graph.
    *  @param _from The processor's output.
    **/
    static void disconnect(AutoPtr<ProcessorOutput>& _from);

    /**
     *  Duplicate a set of nodes.
     *  All connections that are outside of this set are not duplicated.
     *  @param _subset The list of processors to duplicate.
     *  @return A new graph which contains all those processors.
     **/
    AutoPtr<ProcessingGraph> copySubset(const std::vector<AutoPtr<Processor>>& _subset);

    /**
     *  Move all the processors inside a new graph.
     *  All connections that are outside of this graph are connected to the new processor.
     *  @param _subset The list of processors to collapse.
     *  @return A new graph which contains all those processors.
     **/
    AutoPtr<ProcessingGraph> collapseSubset(const std::vector<AutoPtr<Processor>>& _subset);

    /**
     *  Expand a duplicated or collapsed graph.
     *  @param _graph The graph to expand.
     *  @param _position Where the graph has to expand.
     **/
    void expandGraph(AutoPtr<ProcessingGraph>& _graph, ImVec2 _position);

    static bool areConnected(Processor * _from, Processor * _to);
    
    void addProxy(AutoPtr<ProcessorOutput> _proxy_o, AutoPtr<ProcessorInput> _proxy_i)
    {
      m_group_outputs.emplace_back(_proxy_o, _proxy_i);
    }

    void addProxy(AutoPtr<ProcessorInput> _proxy_i, AutoPtr<ProcessorOutput> _proxy_o)
    {
      m_group_inputs.emplace_back(_proxy_i, _proxy_o);
    }

    /**
     *  Get the list of processors in the graph.
     *  @return The list of processors within the graph.
     */
    const std::vector<AutoPtr<Processor>> processors()
    {
      return m_processors;
    }
    
  public: 
    //bool draw();

    AutoPtr<Processor> clone() {
      return AutoPtr<Processor>(new ProcessingGraph(*this));
    }

    void save(std::ofstream& _stream);

    void iceSL(std::ofstream& _stream);

    bool isDirty() {
      for (AutoPtr<Processor> processor : m_processors) {
        if (processor->isDirty()) {
          return true;
        }
      }
      return false;
    }
    
    ImVec2 getBarycenter(std::vector<AutoPtr<Processor>> _processors) {
      size_t nb_proc = _processors.size();
      ImVec2 position(0, 0);

      // Add all processors to the new graph
      for (AutoPtr<Processor> processor : _processors) {
        position += processor->getPosition();
      }
      position /= (float)nb_proc;

      return position;
    }

    ImVec2 getBarycenter() {
      return getBarycenter(m_processors);
    }

    AABox getBoundingBox(std::vector<AutoPtr<Processor>> _processors) {
      size_t nb_proc = _processors.size();
      AABox bbox;

      // Add all processors to the new graph
      for (AutoPtr<Processor> processor : _processors) {
        bbox.m_Mins[0] = std::min(bbox.m_Mins[0], processor->getPosition().x);
        bbox.m_Mins[1] = std::min(bbox.m_Mins[1], processor->getPosition().y);
        bbox.m_Maxs[0] = std::max(bbox.m_Maxs[0], processor->getPosition().x);
        bbox.m_Maxs[1] = std::max(bbox.m_Maxs[1], processor->getPosition().y);
      }

      return bbox;
    }
  };
}