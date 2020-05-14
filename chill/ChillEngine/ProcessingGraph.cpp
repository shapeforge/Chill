#include "ProcessingGraph.h"

#include <map>

namespace chill {
  
  template <class T> using pairOf = std::pair<T, T>;

  ProcessingGraph::ProcessingGraph(ProcessingGraph &copy) {
    
    setName (copy.name());
    setColor(copy.color());
    setOwner(copy.owner());

    //clone inputs
    std::vector<pairOf<ProcessorInputPtr>> input_old_new;
    for (ProcessorInputPtr input : copy.inputs()) {
      ProcessorInputPtr new_input = input->clone();
      input_old_new.push_back(std::make_pair(input, new_input));
      addInput(new_input);
    }

    // clone outputs
    std::vector<pairOf<ProcessorOutputPtr>> output_old_new;
    for (ProcessorOutputPtr output : copy.outputs()) {
      ProcessorOutputPtr new_output = output->clone();
      output_old_new.push_back(std::make_pair(output, new_output));
      addOutput(new_output);
    }

    // clone nodes
    std::vector<pairOf<ProcessorPtr>> proc_old_new;
    for (auto processor : copy.m_processors) {
      auto new_proc = std::static_pointer_cast<Processor>(processor->clone());
      new_proc->setPosition(processor->getPosition());
      proc_old_new.push_back(std::make_pair(processor, new_proc));
      addProcessor(new_proc);
    }

    // recreate the pipes
    for (ProcessorPtr processor : copy.m_processors) {
      for (ProcessorInputPtr input : processor->inputs()) {
        if (!input || !input->m_link) continue;

        auto output = input->m_link;

        auto oldInputProcessor = input->owner();
        auto oldOutputProcessor = output->owner();

        auto it1 = std::find_if(proc_old_new.begin(), proc_old_new.end(),
          [oldInputProcessor](const pairOf<ProcessorPtr>& element) { return element.first.get() == oldInputProcessor; });
        auto it2 = std::find_if(proc_old_new.begin(), proc_old_new.end(),
          [oldOutputProcessor](const pairOf<ProcessorPtr>& element) { return element.first.get() == oldOutputProcessor; });

        if (it1 == proc_old_new.end() || it2 == proc_old_new.end()) continue;

        ProcessorPtr new1 = it1->second;
        ProcessorPtr new2 = it2->second;

        connect(new2->output(output->name()), new1->input(input->name()));
      }
    }

    // recreate GroupIO
    for (GroupInput ginput : copy.m_group_inputs) {
      auto it_ginput = std::find_if(input_old_new.begin(), input_old_new.end(),
        [ginput](const pairOf<ProcessorInputPtr>& element) { return element.first == ginput.first; });
      if (it_ginput == input_old_new.end()) continue;
      
      ProcessorInputPtr old_exposed_input = it_ginput->first;
      ProcessorInputPtr new_exposed_input = it_ginput->second;
      
      Processor* old_groupprocessor = old_exposed_input->owner();
      auto it_proc = std::find_if(proc_old_new.begin(), proc_old_new.end(),
        [old_groupprocessor](const pairOf<ProcessorPtr>& element) { return element.first.get() == old_groupprocessor; });
      if (it_proc == proc_old_new.end()) continue;
      
      ProcessorPtr new_groupprocessor = it_proc->second;

      ProcessorOutputPtr new_groupinput = new_groupprocessor->output(new_exposed_input->name());

      m_group_inputs.push_back(GroupInput(new_exposed_input, new_groupinput));
    }
  }

  ProcessingGraph::~ProcessingGraph() {
    m_group_inputs.clear();
    m_group_outputs.clear();

    // NOTE: hotfix : this shouldn't be needed with shared_ptr
    /*
    for (ProcessorPtr processor : m_processors) {
      remove(processor);
    }
    for (std::shared_ptr<VisualComment> element : m_comments) {
      remove(element);
    }
    */
  }

  void ProcessingGraph::remove(ProcessorPtr _processor) {
    // disconnect
    if (_processor->owner() == this) {
      for (ProcessorInputPtr input : _processor->inputs()) {
        if (input) {
          disconnect(input);
        }
      }
      for (ProcessorOutputPtr output : _processor->outputs()) {
        if (output) {
          disconnect(output);
        }
      }
    }

    // remove the processor
    m_processors.erase(std::remove(m_processors.begin(), m_processors.end(), _processor), m_processors.end());
    //_processor.~std::shared_ptr();
  }

  void ProcessingGraph::remove(std::shared_ptr<VisualComment> _comment) {
    m_comments.erase(std::remove(m_comments.begin(), m_comments.end(), _comment), m_comments.end());
  }

  void ProcessingGraph::remove(std::shared_ptr<SelectableUI> _select) {
    ProcessorPtr proc = std::static_pointer_cast<Processor>(_select);
    if (proc) {
      remove(proc);
    }
    std::shared_ptr<VisualComment> com = std::static_pointer_cast<VisualComment>(_select);
    if (com) {
      remove(com);
    }
  }




  std::shared_ptr<ProcessingGraph> ProcessingGraph::collapseSubset(const std::vector<std::shared_ptr<SelectableUI>>& subset)
  {
    std::shared_ptr<ProcessingGraph> innerGraph   = addProcessor<ProcessingGraph>();
    std::shared_ptr<GroupProcessor>  groupInputs  = innerGraph->addProcessor<GroupProcessor>();
    std::shared_ptr<GroupProcessor>  groupOutputs = innerGraph->addProcessor<GroupProcessor>();

    innerGraph->setName("group");
    groupInputs->setName("Group Input");
    groupInputs->setInputMode(true);
    groupOutputs->setName("Group Output");
    groupOutputs->setOutputMode(true);

    // move the entities
    for (std::shared_ptr<SelectableUI> select : subset) {
      innerGraph->add(select);
    }

    // edit border pipes
    for (std::shared_ptr<SelectableUI> select : subset) {
      ProcessorPtr processor = std::static_pointer_cast<Processor>(select);
      if (!processor) {
        continue;
      }

      for (ProcessorInputPtr input : processor->inputs()) {
        ProcessorOutputPtr output = input->m_link;
        // not linked
        if (!output) continue;
        // same graph, nothing to do
        if (output->owner()->owner() == input->owner()->owner()) continue;

        auto it = std::find_if(innerGraph->m_group_inputs.begin(), innerGraph->m_group_inputs.end(),
          [output](const std::pair<ProcessorInputPtr, ProcessorOutputPtr>& element) {
            return element.first->m_link.get() == output.get();
          });

        if (it != innerGraph->m_group_inputs.end()) {
          ProcessorOutputPtr groupInput = it->second;
          connect(groupInput, input);
          continue;
        }

        ProcessorInputPtr exposedInput = innerGraph->addInput(input->name(), input->type());
        ProcessorOutputPtr groupInput = groupInputs->addOutput(exposedInput->name(), input->type());

        innerGraph->m_group_inputs.push_back(GroupInput(exposedInput, groupInput));

        disconnect(input);
        connect(output, exposedInput);
        connect(groupInput, input);
      }


      for (ProcessorOutputPtr output : processor->outputs()) {
        bool linked = false;
        
        for (ProcessorInputPtr input : output->m_links) {
          // not linked
          if (!input) continue;
          // same graph, nothing to do
          if (input->owner()->owner() == output->owner()->owner()) continue;

          linked = true;
          break;
        }

        if (linked) {

          std::string base = output->name();
          std::string name = base;
          int nb = 1;
          while (innerGraph->output(name)) {
            name = base + "_" + std::to_string(nb++);
          }

          ProcessorOutputPtr innerOutput = innerGraph->addOutput(name, output->type());
          ProcessorInputPtr innerOutputGroup = groupOutputs->addInput(name, output->type());
          innerGraph->m_group_outputs.emplace_back(innerOutput, innerOutputGroup);

          for (ProcessorInputPtr input : output->m_links) {
            if (input->owner()->owner() != output->owner()->owner()) {
              disconnect(input);
              connect(innerOutput, input);
            }
          }
          connect(output, innerOutputGroup);
        }
      }
    }

    for (std::shared_ptr<SelectableUI> select : subset) {
      remove(select);
    }

    auto bbox = getBoundingBox(subset);
    auto barycenter = getBarycenter(subset);

    innerGraph->setPosition(barycenter);
    groupInputs->setPosition (ImVec2(bbox.m_min[0] - style.processor_width * 2, barycenter.y));
    groupOutputs->setPosition(ImVec2(bbox.m_max[0] + style.processor_width * 2, barycenter.y));
    return innerGraph;
  }

  void ProcessingGraph::expandGraph(std::shared_ptr<ProcessingGraph> collapsed, ImVec2 position) {

    // Move processors
    for (ProcessorPtr processor : collapsed->m_processors) {
      std::shared_ptr<GroupProcessor> proc = std::static_pointer_cast<GroupProcessor> (processor);
      // If the processor is not a GroupProcessor
      if (proc) {
        addProcessor(processor);
      }
    }

    for (std::shared_ptr<VisualComment> comment : collapsed->m_comments) {
        addComment(comment);
    }
    
    // Update pipes
    for (GroupInput gi : collapsed->m_group_inputs) {
      ProcessorOutputPtr output = gi.first->m_link;
      for (ProcessorInputPtr input : gi.second->m_links) {
        connect(output, input);
      }
    }
    for (GroupOutput go : collapsed->m_group_outputs) {
      ProcessorOutputPtr output = go.second->m_link;
      for (ProcessorInputPtr input : go.first->m_links) {
      }
    }

    ImVec2 offset = position - collapsed->getBarycenter();
    for (ProcessorPtr processor : collapsed->m_processors) {
      processor->setPosition(processor->getPosition() + offset);
    }

    remove(std::static_pointer_cast<Processor>(collapsed));
  }

  std::shared_ptr<ProcessingGraph> ProcessingGraph::copySubset(std::vector<std::shared_ptr<SelectableUI>>& subset)
  {
    std::shared_ptr<ProcessingGraph> graph = std::shared_ptr<ProcessingGraph>(new ProcessingGraph());
    
    std::vector<pairOf<ProcessorPtr>> assoc;
    
    // copy the node
    for (std::shared_ptr<SelectableUI> select : subset) {
      std::shared_ptr<SelectableUI> new_select = select->clone();
      new_select->setOwner(nullptr);
      new_select->setPosition(select->getPosition());

      ProcessorPtr processor = std::static_pointer_cast<Processor>(select);
      ProcessorPtr new_processor = std::static_pointer_cast<Processor>(new_select);
      if (processor && new_processor) {
        assoc.push_back(std::make_pair(processor, new_processor));
      }
      graph->add(new_select);
    }

    // recreate the pipes
    for (std::shared_ptr<SelectableUI> select : subset) {
      ProcessorPtr processor = std::static_pointer_cast<Processor>(select);
      if (!processor) {
        continue;
      }
      for (ProcessorInputPtr input : processor->inputs()) {
        if (!input) continue;
        if (!input->m_link) continue;

        ProcessorOutputPtr output = input->m_link;

        Processor* old1 = input->owner();
        Processor* old2 = output->owner();

        auto it1 = std::find_if(assoc.begin(), assoc.end(),
          [old1](const pairOf<ProcessorPtr>& element) { return element.first.get() == old1; });
        auto it2 = std::find_if(assoc.begin(), assoc.end(),
          [old2](const pairOf<ProcessorPtr>& element) { return element.first.get() == old2; });

        if (it1 == assoc.end() || it2 == assoc.end()) continue;

        ProcessorPtr new1 = it1->second;
        ProcessorPtr new2 = it2->second;
        
        connect(new2->output(output->name()), new1->input(input->name()));
      }
    }

    return graph;
  }

  void ProcessingGraph::save(std::ofstream& _stream) {
    _stream << "p_" << getUniqueID() << " = Graph('" << name() << "')" << std::endl;
    
    ImVec2 bar = getBarycenter();
    // Save the nodes
    for (ProcessorPtr proc : m_processors) {
      proc->translate(ImVec2(0,0)-bar);
      proc->save(_stream);
      proc->translate(bar);
      _stream << "p_" << getUniqueID() << ":add( p_" << proc->getUniqueID() << ")" << std::endl;
    }

    // Save the connections
    for (ProcessorPtr proc : m_processors) {
      for (ProcessorInputPtr input : proc->inputs()) {
        ProcessorOutputPtr output = input->m_link;
        if (!output) continue;
        _stream << "connect( o_" << output->getUniqueID() << ", i_" << input->getUniqueID() << ")" << std::endl;
      }
    }
    _stream << "set_graph(p_" << getUniqueID() << ")" << std::endl;
  }

  void ProcessingGraph::iceSL(std::ofstream& _stream) {
    std::set<Processor*> done;
    std::unordered_set<Processor*> toDo;

    for (ProcessorPtr processor : m_processors) {
      bool not_connected = true;
      for (ProcessorInputPtr input : processor->inputs()) {
        if (input && input->m_link) {
          not_connected = false;
          break;
        }
      }
      if (not_connected) {
        toDo.emplace(processor.get());
      }
    }

    _stream << "--[[ " + std::string(name()) + " ]]--" << std::endl <<
               "setfenv(1, _G0)  --go back to global initialization" << std::endl <<
               "__currentNodeId = " << std::to_string(reinterpret_cast<int64_t>(this)) << std::endl;

    if ( (owner() != nullptr && owner()->isDirty()) || isDirty() || isEmiter()) {
      _stream << "setDirty(__currentNodeId)" << std::endl;
    }

    _stream << "if (isDirty({__currentNodeId";

    for (ProcessorInputPtr input : inputs()) {
      if (input->m_link) {
        std::string s2 = std::to_string(reinterpret_cast<int64_t>(input->m_link->owner()));
        _stream << ", " + s2;
      }
    }

    _stream << "})) then" << std::endl
            << "setDirty(__currentNodeId)" << std::endl
            << "end" << std::endl;

    while (!toDo.empty()) {
      Processor* processor = *toDo.begin();
      toDo.erase(toDo.begin());
      processor->iceSL(_stream);
      done.emplace(processor);

      for (ProcessorOutputPtr output : processor->outputs()) {
        for (ProcessorInputPtr nextInput : output->m_links) {
          bool all_inputs_done = true;
          for (ProcessorInputPtr input : nextInput->owner()->inputs()) {
            if (input && input->m_link) {
              auto p = done.find(input->m_link->owner());
              if (p == done.end()) {
                all_inputs_done = false;
                break;
              }
            }
          }
          if (all_inputs_done) {
            toDo.emplace(nextInput->owner());
          }
        }
      }
    }

    toDo.clear();
    done.clear();

    _stream << "--[[ ! " + std::string(name()) + " ]]--\n" << std::endl;
  }
}
