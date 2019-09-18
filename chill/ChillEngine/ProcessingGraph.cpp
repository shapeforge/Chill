#include "ProcessingGraph.h"


namespace chill {

  ProcessingGraph::ProcessingGraph(ProcessingGraph &copy) {
    setName (copy.name());
    setColor(copy.color());
    setOwner(copy.owner());

    //clone inputs
    std::vector<std::pair<std::shared_ptr<ProcessorInput>, std::shared_ptr<ProcessorInput>>> input_old_new;
    for (std::shared_ptr<ProcessorInput> input : copy.inputs()) {
      std::shared_ptr<ProcessorInput> new_input = input->clone();
      input_old_new.push_back(std::make_pair(input, new_input));
      addInput(new_input);
    }

    // clone outputs
    std::vector<std::pair<std::shared_ptr<ProcessorOutput>, std::shared_ptr<ProcessorOutput>>> output_old_new;
    for (std::shared_ptr<ProcessorOutput> output : copy.outputs()) {
      std::shared_ptr<ProcessorOutput> new_output = output->clone();
      output_old_new.push_back(std::make_pair(output, new_output));
      addOutput(new_output);
    }

    // clone nodes
    std::vector<std::pair<std::shared_ptr<Processor>, std::shared_ptr<Processor>>> proc_old_new;
    for (std::shared_ptr<Processor> processor : copy.m_processors) {
      std::shared_ptr<Processor> new_proc = std::static_pointer_cast<Processor>(processor->clone());
      new_proc->setPosition(processor->getPosition());
      proc_old_new.push_back(std::make_pair(processor, new_proc));
      addProcessor(new_proc);
    }

    // recreate the pipes
    for (std::shared_ptr<Processor> processor : copy.m_processors) {
      for (std::shared_ptr<ProcessorInput> input : processor->inputs()) {
        if (!input) continue;
        if (!input->m_link) continue;

        std::shared_ptr<ProcessorOutput> output = input->m_link;

        Processor* old1 = input->owner();
        Processor* old2 = output->owner();

        auto it1 = std::find_if(proc_old_new.begin(), proc_old_new.end(),
          [old1](const std::pair<std::shared_ptr<Processor>, std::shared_ptr<Processor>>& element) { return element.first.get() == old1; });
        auto it2 = std::find_if(proc_old_new.begin(), proc_old_new.end(),
          [old2](const std::pair<std::shared_ptr<Processor>, std::shared_ptr<Processor>>& element) { return element.first.get() == old2; });

        if (it1 == proc_old_new.end() || it2 == proc_old_new.end()) continue;

        std::shared_ptr<Processor> new1 = it1->second;
        std::shared_ptr<Processor> new2 = it2->second;

        connect(new2->output(output->name()), new1->input(input->name()));
      }
    }

    // recreate GroupIO
    for (GroupInput ginput : copy.m_group_inputs) {
      auto it_ginput = std::find_if(input_old_new.begin(), input_old_new.end(),
        [ginput](const std::pair<std::shared_ptr<ProcessorInput>, std::shared_ptr<ProcessorInput>>& element) { return element.first == ginput.first; });
      if (it_ginput == input_old_new.end()) continue;
      
      std::shared_ptr<ProcessorInput> old_exposed_input = it_ginput->first;
      std::shared_ptr<ProcessorInput> new_exposed_input = it_ginput->second;
      
      Processor* old_groupprocessor = old_exposed_input->owner();
      auto it_proc = std::find_if(proc_old_new.begin(), proc_old_new.end(),
        [old_groupprocessor](const std::pair<std::shared_ptr<Processor>, std::shared_ptr<Processor>>& element) { return element.first.get() == old_groupprocessor; });
      if (it_proc == proc_old_new.end()) continue;
      
      std::shared_ptr<Processor> new_groupprocessor = it_proc->second;

      std::shared_ptr<ProcessorOutput> new_groupinput = new_groupprocessor->output(new_exposed_input->name());

      m_group_inputs.push_back(GroupInput(new_exposed_input, new_groupinput));
    }
  }

  ProcessingGraph::~ProcessingGraph() {
    m_group_inputs.clear();
    m_group_outputs.clear();

    // NOTE: hotfix : this shouldn't be needed with shared_ptr
    /*
    for (std::shared_ptr<Processor> processor : m_processors) {
      remove(processor);
    }
    for (std::shared_ptr<VisualComment> element : m_comments) {
      remove(element);
    }
    */
  }

  void ProcessingGraph::remove(std::shared_ptr<Processor> _processor) {
    // disconnect
    if (_processor->owner() == this) {
      for (std::shared_ptr<ProcessorInput> input : _processor->inputs()) {
        if (input) {
          disconnect(input);
        }
      }
      for (std::shared_ptr<ProcessorOutput> output : _processor->outputs()) {
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
    std::shared_ptr<Processor> proc = std::static_pointer_cast<Processor>(_select);
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
      std::shared_ptr<Processor> processor = std::static_pointer_cast<Processor>(select);
      if (!processor) {
        continue;
      }

      for (std::shared_ptr<ProcessorInput> input : processor->inputs()) {
        std::shared_ptr<ProcessorOutput> output = input->m_link;
        // not linked
        if (!output) continue;
        // same graph, nothing to do
        if (output->owner()->owner() == input->owner()->owner()) continue;

        auto it = std::find_if(innerGraph->m_group_inputs.begin(), innerGraph->m_group_inputs.end(),
          [output](const std::pair<std::shared_ptr<ProcessorInput>, std::shared_ptr<ProcessorOutput>>& element) {
            return element.first->m_link.get() == output.get();
          });

        if (it != innerGraph->m_group_inputs.end()) {
          std::shared_ptr<ProcessorOutput> groupInput = it->second;
          connect(groupInput, input);
          continue;
        }

        std::shared_ptr<ProcessorInput> exposedInput = innerGraph->addInput(input->name(), input->type());
        std::shared_ptr<ProcessorOutput> groupInput = groupInputs->addOutput(exposedInput->name(), input->type());

        innerGraph->m_group_inputs.push_back(GroupInput(exposedInput, groupInput));

        disconnect(input);
        connect(output, exposedInput);
        connect(groupInput, input);
      }


      for (std::shared_ptr<ProcessorOutput> output : processor->outputs()) {
        bool linked = false;
        
        for (std::shared_ptr<ProcessorInput> input : output->m_links) {
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

          std::shared_ptr<ProcessorOutput> innerOutput = innerGraph->addOutput(name, output->type());
          std::shared_ptr<ProcessorInput> innerOutputGroup = groupOutputs->addInput(name, output->type());
          innerGraph->m_group_outputs.emplace_back(innerOutput, innerOutputGroup);

          for (std::shared_ptr<ProcessorInput> input : output->m_links) {
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

    AABox bbox = getBoundingBox(subset);
    ImVec2 barycenter = getBarycenter(subset);

    innerGraph->setPosition(barycenter);
    groupInputs->setPosition (ImVec2(bbox.minCorner()[0] - style.processor_width * 2, barycenter.y));
    groupOutputs->setPosition(ImVec2(bbox.maxCorner()[0] + style.processor_width * 2, barycenter.y));
    return innerGraph;
  }

  void ProcessingGraph::expandGraph(std::shared_ptr<ProcessingGraph> collapsed, ImVec2 position) {

    // Move processors
    for (std::shared_ptr<Processor> processor : collapsed->m_processors) {
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
      std::shared_ptr<ProcessorOutput> output = gi.first->m_link;
      for (std::shared_ptr<ProcessorInput> input : gi.second->m_links) {
        connect(output, input);
      }
    }
    for (GroupOutput go : collapsed->m_group_outputs) {
      std::shared_ptr<ProcessorOutput> output = go.second->m_link;
      for (std::shared_ptr<ProcessorInput> input : go.first->m_links) {
        connect(output, input);
      }
    }

    ImVec2 offset = position - collapsed->getBarycenter();
    for (std::shared_ptr<Processor> processor : collapsed->m_processors) {
      processor->setPosition(processor->getPosition() + offset);
    }

    remove(std::static_pointer_cast<Processor>(collapsed));
  }

  std::shared_ptr<ProcessingGraph> ProcessingGraph::copySubset(std::vector<std::shared_ptr<SelectableUI>>& subset)
  {
    std::shared_ptr<ProcessingGraph> graph = std::shared_ptr<ProcessingGraph>(new ProcessingGraph());
    
    std::vector<std::pair<std::shared_ptr<Processor>, std::shared_ptr<Processor>>> assoc;
    
    // copy the node
    for (std::shared_ptr<SelectableUI> select : subset) {
      std::shared_ptr<SelectableUI> new_select = select->clone();
      new_select->setOwner(nullptr);
      new_select->setPosition(select->getPosition());

      std::shared_ptr<Processor> processor = std::static_pointer_cast<Processor>(select);
      std::shared_ptr<Processor> new_processor = std::static_pointer_cast<Processor>(new_select);
      if (processor && new_processor) {
        assoc.push_back(std::make_pair(processor, new_processor));
      }
      graph->add(new_select);
    }

    // recreate the pipes
    for (std::shared_ptr<SelectableUI> select : subset) {
      std::shared_ptr<Processor> processor = std::static_pointer_cast<Processor>(select);
      if (!processor) {
        continue;
      }
      for (std::shared_ptr<ProcessorInput> input : processor->inputs()) {
        if (!input) continue;
        if (!input->m_link) continue;

        std::shared_ptr<ProcessorOutput> output = input->m_link;

        Processor* old1 = input->owner();
        Processor* old2 = output->owner();

        auto it1 = std::find_if(assoc.begin(), assoc.end(),
          [old1](const std::pair<std::shared_ptr<Processor>, std::shared_ptr<Processor>>& element) { return element.first.get() == old1; });
        auto it2 = std::find_if(assoc.begin(), assoc.end(),
          [old2](const std::pair<std::shared_ptr<Processor>, std::shared_ptr<Processor>>& element) { return element.first.get() == old2; });

        if (it1 == assoc.end() || it2 == assoc.end()) continue;

        std::shared_ptr<Processor> new1 = it1->second;
        std::shared_ptr<Processor> new2 = it2->second;
        
        connect(new2->output(output->name()), new1->input(input->name()));
      }
    }

    return graph;
  }

  void ProcessingGraph::save(std::ofstream& _stream) {
    _stream << "p_" << getUniqueID() << " = Graph('" << name() << "')" << std::endl;
    
    ImVec2 bar = getBarycenter();
    // Save the nodes
    for (std::shared_ptr<Processor> proc : m_processors) {
      proc->translate(ImVec2(0,0)-bar);
      proc->save(_stream);
      proc->translate(bar);
      _stream << "p_" << getUniqueID() << ":add( p_" << proc->getUniqueID() << ")" << std::endl;
    }

    // Save the connections
    for (std::shared_ptr<Processor> proc : m_processors) {
      for (std::shared_ptr<ProcessorInput> input : proc->inputs()) {
        std::shared_ptr<ProcessorOutput> output = input->m_link;
        if (!output) continue;
        _stream << "connect( o_" << output->getUniqueID() << ", i_" << input->getUniqueID() << ")" << std::endl;
      }
    }
    _stream << "set_graph(p_" << getUniqueID() << ")" << std::endl;
  }

  void ProcessingGraph::iceSL(std::ofstream& _stream) {
    std::set<Processor*> done;
    std::unordered_set<Processor*> toDo;

    for (std::shared_ptr<Processor> processor : m_processors) {
      bool not_connected = true;
      for (std::shared_ptr<ProcessorInput> input : processor->inputs()) {
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

    for (auto input : inputs()) {
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

      for (std::shared_ptr<ProcessorOutput> output : processor->outputs()) {
        for (std::shared_ptr<ProcessorInput> nextInput : output->m_links) {
          bool all_inputs_done = true;
          for (std::shared_ptr<ProcessorInput> input : nextInput->owner()->inputs()) {
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
