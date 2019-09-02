/** @file */
#pragma once

// Processor.h
namespace Chill
{
  class Processor;
  class VisualComment;
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

#include "VisualComment.h"



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
    /** List of all the comments */
    std::vector<AutoPtr<VisualComment>> m_comments;

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
     *  Add an existing comment to the graph.
     *  @param _visualComment The AutoPtr related to the comment.
     **/
    void addComment(AutoPtr<VisualComment>& _visualComment) {
      sl_assert(&_visualComment);
      sl_assert(_visualComment->owner() != this);

      _visualComment->setOwner(this);
      m_comments.push_back(_visualComment);
    }

    /**
     *  Add an existing object to the graph.
     *  @param _object The AutoPtr related to the object.
     **/
    void add(AutoPtr<SelectableUI>& _object) {
      AutoPtr<Processor> proc = AutoPtr<Processor>(_object);
      if (!proc.isNull()) {
        addProcessor(proc);
      }
      AutoPtr<VisualComment> com = AutoPtr<VisualComment>(_object);
      if (!com.isNull()) {
        addComment(com);
      }
    }

    /**
     *  Remove an existing processor from the graph.
     *  @param _processor The AutoPtr related to the processor.
     **/
    void remove(AutoPtr<Processor> _processor);

    /**
     *  Remove an existing comment from the graph.
     *  @param _comment The AutoPtr related to the comment.
     **/
    void remove(AutoPtr<VisualComment> _comment);

    /**
     *  Remove an existing _object from the graph.
     *  @param _object The AutoPtr related to the object.
     **/
    void remove(AutoPtr<SelectableUI> _object);

    /**
     *  Duplicate a set of nodes.
     *  All connections that are outside of this set are not duplicated.
     *  @param _subset The list of processors to duplicate.
     *  @return A new graph which contains all those processors.
     **/
    AutoPtr<ProcessingGraph> copySubset(std::vector<AutoPtr<SelectableUI>>& _subset);

    /**
     *  Move all the processors inside a new graph.
     *  All connections that are outside of this graph are connected to the new processor.
     *  @param _subset The list of processors to collapse.
     *  @return A new graph which contains all those processors.
     **/
    AutoPtr<ProcessingGraph> collapseSubset(const std::vector<AutoPtr<SelectableUI>>& _subset);

    /**
     *  Expand a duplicated or collapsed graph.
     *  @param _graph The graph to expand.
     *  @param _position Where the graph has to expand.
     **/
    void expandGraph(AutoPtr<ProcessingGraph> _graph, ImVec2 _position);
    
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

    /**
     *  Get the list of comments in the graph.
     *  @return The list of comments within the graph.
     */
    const std::vector<AutoPtr<VisualComment>> comments()
    {
      return m_comments;
    }
    
  public: 
    //bool draw();

    AutoPtr<SelectableUI> clone() {
      return AutoPtr<SelectableUI>(new ProcessingGraph(*this));
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
    
    ImVec2 getBarycenter(std::vector<AutoPtr<SelectableUI>> _elements) {
      ImVec2 position(0, 0);
      for (AutoPtr<SelectableUI> processor : _elements) {
        position += processor->getPosition();
      }
      return position / (float)_elements.size();
    }

    ImVec2 getBarycenter() {
      std::vector<AutoPtr<SelectableUI>> list;
      for (auto &proc : m_processors) {
          list.push_back(AutoPtr<SelectableUI>(proc));
      }
      for (auto &com : m_comments) {
          list.push_back(AutoPtr<SelectableUI>(com));
      }
//      list.insert(list.end(), m_processors.begin(), m_processors.end());
//      list.insert(list.end(), m_comments.begin(), m_comments.end());
      return getBarycenter(list);
    }

    AABox getBoundingBox(std::vector<AutoPtr<SelectableUI>> _elements) {
      AABox bbox;
      for (AutoPtr<SelectableUI> element : _elements) {
        bbox.m_Mins[0] = std::min(bbox.m_Mins[0], element->getPosition().x);
        bbox.m_Mins[1] = std::min(bbox.m_Mins[1], element->getPosition().y);
        bbox.m_Maxs[0] = std::max(bbox.m_Maxs[0], element->getPosition().x);
        bbox.m_Maxs[1] = std::max(bbox.m_Maxs[1], element->getPosition().y);
      }
      return bbox;
    }
  };
}
