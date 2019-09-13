/** @file */
#pragma once


#include <set>
#include <unordered_set>
#include <vector>

#include <LibSL.h>

#include "IOs.h"
#include "Processor.h"
#include "UI.h"
#include "VisualComment.h"

namespace chill {
class ProcessingGraph;
class Processor;
class ProcessorInput;
class ProcessorOutput;
class VisualComment;
};

namespace chill {

/**
 * Graph class. Contains all nodes and connections.
 */
class ProcessingGraph : public Processor
{
  private:
    // inner_input -> visible_output
    typedef std::pair<std::shared_ptr<ProcessorInput>, std::shared_ptr<ProcessorOutput>> GroupInput;
    // inner_output -> visible_input
    typedef std::pair<std::shared_ptr<ProcessorOutput>, std::shared_ptr<ProcessorInput>> GroupOutput;

  protected:
    /** List of all processors within this graph. */
    std::vector<std::shared_ptr<Processor>> m_processors;
    /** List of all entry points */
    std::vector<GroupInput>         m_group_inputs;
    /** List of all exit points */
    std::vector<GroupOutput>        m_group_outputs;
    /** List of all the comments */
    std::vector<std::shared_ptr<VisualComment>> m_comments;

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
     *  @return The std::shared_ptr related to the processor created.
     **/
    template<typename T_Processor, typename... Args>
    std::shared_ptr<T_Processor> addProcessor(Args&& ... args)
    {
      std::shared_ptr<T_Processor> processor(new T_Processor(args...));
      processor->setOwner(this);
      m_processors.push_back(static_cast<std::shared_ptr<Processor>>(processor));
      return processor;
    }

    /**
     *  Add an existing processor to the graph.
     *  @param _processor The std::shared_ptr related to the processor.
     **/
    void addProcessor(std::shared_ptr<Processor>& _processor)
    {
      sl_assert(&_processor);
      sl_assert(_processor->owner() != this);

      _processor->setOwner(this);
      m_processors.push_back(_processor);
    }


    /**
     *  Add an existing comment to the graph.
     *  @param _visualComment The std::shared_ptr related to the comment.
     **/
    void addComment(std::shared_ptr<VisualComment>& _visualComment) {
      sl_assert(&_visualComment);
      sl_assert(_visualComment->owner() != this);

      _visualComment->setOwner(this);
      m_comments.push_back(_visualComment);
    }

    /**
     *  Add an existing object to the graph.
     *  @param _object The std::shared_ptr related to the object.
     **/
    void add(std::shared_ptr<SelectableUI>& _object) {
      std::shared_ptr<Processor> proc = std::static_pointer_cast<Processor>(_object);
      if (proc) {
        addProcessor(proc);
      }
      std::shared_ptr<VisualComment> com = std::static_pointer_cast<VisualComment>(_object);
      if (com) {
        addComment(com);
      }
    }

    /**
     *  Remove an existing processor from the graph.
     *  @param _processor The std::shared_ptr related to the processor.
     **/
    void remove(std::shared_ptr<Processor> _processor);

    /**
     *  Remove an existing comment from the graph.
     *  @param _comment The std::shared_ptr related to the comment.
     **/
    void remove(std::shared_ptr<VisualComment> _comment);

    /**
     *  Remove an existing _object from the graph.
     *  @param _object The std::shared_ptr related to the object.
     **/
    void remove(std::shared_ptr<SelectableUI> _object);

    /**
     *  Duplicate a set of nodes.
     *  All connections that are outside of this set are not duplicated.
     *  @param _subset The list of processors to duplicate.
     *  @return A new graph which contains all those processors.
     **/
    std::shared_ptr<ProcessingGraph> copySubset(std::vector<std::shared_ptr<SelectableUI>>& _subset);

    /**
     *  Move all the processors inside a new graph.
     *  All connections that are outside of this graph are connected to the new processor.
     *  @param _subset The list of processors to collapse.
     *  @return A new graph which contains all those processors.
     **/
    std::shared_ptr<ProcessingGraph> collapseSubset(const std::vector<std::shared_ptr<SelectableUI>>& _subset);

    /**
     *  Expand a duplicated or collapsed graph.
     *  @param _graph The graph to expand.
     *  @param _position Where the graph has to expand.
     **/
    void expandGraph(std::shared_ptr<ProcessingGraph> _graph, ImVec2 _position);
    
    void addProxy(std::shared_ptr<ProcessorOutput> _proxy_o, std::shared_ptr<ProcessorInput> _proxy_i)
    {
      m_group_outputs.emplace_back(_proxy_o, _proxy_i);
    }

    void addProxy(std::shared_ptr<ProcessorInput> _proxy_i, std::shared_ptr<ProcessorOutput> _proxy_o)
    {
      m_group_inputs.emplace_back(_proxy_i, _proxy_o);
    }

    /**
     *  Get the list of processors in the graph.
     *  @return The list of processors within the graph.
     */
    const std::vector<std::shared_ptr<Processor>> processors()
    {
      return m_processors;
    }

    /**
     *  Get the list of comments in the graph.
     *  @return The list of comments within the graph.
     */
    const std::vector<std::shared_ptr<VisualComment>> comments()
    {
      return m_comments;
    }
    
  public:
    //bool draw();

    std::shared_ptr<SelectableUI> clone() {
      return std::shared_ptr<SelectableUI>(new ProcessingGraph(*this));
    }

    void save(std::ofstream& _stream);

    void iceSL(std::ofstream& _stream);

    bool isDirty() {
      for (std::shared_ptr<Processor> processor : m_processors) {
        if (processor->isDirty()) {
          return true;
        }
      }
      return false;
    }
    
    ImVec2 getBarycenter(std::vector<std::shared_ptr<SelectableUI>> _elements) {
      ImVec2 position(0, 0);
      for (std::shared_ptr<SelectableUI> processor : _elements) {
        position += processor->getPosition();
      }
      return position / static_cast<float>(_elements.size());
    }

    ImVec2 getBarycenter() {
      std::vector<std::shared_ptr<SelectableUI>> list;
      for (auto &proc : m_processors) {
        list.push_back(std::shared_ptr<SelectableUI>(proc));
      }
      for (auto &com : m_comments) {
        list.push_back(std::shared_ptr<SelectableUI>(com));
      }
      //      list.insert(list.end(), m_processors.begin(), m_processors.end());
      //      list.insert(list.end(), m_comments.begin(), m_comments.end());
      return getBarycenter(list);
    }

    AABox getBoundingBox(std::vector<std::shared_ptr<SelectableUI>> _elements) {
      AABox bbox;
      for (std::shared_ptr<SelectableUI> element : _elements) {
        bbox.m_Mins[0] = std::min(bbox.m_Mins[0], element->getPosition().x);
        bbox.m_Mins[1] = std::min(bbox.m_Mins[1], element->getPosition().y);
        bbox.m_Maxs[0] = std::max(bbox.m_Maxs[0], element->getPosition().x);
        bbox.m_Maxs[1] = std::max(bbox.m_Maxs[1], element->getPosition().y);
      }
      return bbox;
    }
};
}
