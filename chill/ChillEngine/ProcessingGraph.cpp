#include "ProcessingGraph.h"
#include "IOs.h"

#include <vector>

namespace Chill {

  ProcessingGraph::ProcessingGraph(ProcessingGraph &copy) {
    setName (copy.name());
    setColor(copy.color());
    setOwner(copy.owner());

    //clone inputs
    std::vector<std::pair<AutoPtr<ProcessorInput>, AutoPtr<ProcessorInput>>> input_old_new;
    for (AutoPtr<ProcessorInput> input : copy.inputs()) {
      AutoPtr<ProcessorInput> new_input = input->clone();
      input_old_new.push_back(std::make_pair(input, new_input));
      addInput(new_input);
    }

    // clone outputs
    std::vector<std::pair<AutoPtr<ProcessorOutput>, AutoPtr<ProcessorOutput>>> output_old_new;
    for (AutoPtr<ProcessorOutput> output : copy.outputs()) {
      AutoPtr<ProcessorOutput> new_output = output->clone();
      output_old_new.push_back(std::make_pair(output, new_output));
      addOutput(new_output);
    }

    // clone nodes
    std::vector<std::pair<AutoPtr<Processor>, AutoPtr<Processor>>> proc_old_new;
    for (AutoPtr<Processor> processor : copy.m_processors) {
      AutoPtr<Processor> new_proc = AutoPtr<Processor>(processor->clone());
      new_proc->setPosition(processor->getPosition());
      proc_old_new.push_back(std::make_pair(processor, new_proc));
      addProcessor(new_proc);
    }

    // recreate the pipes
    for (AutoPtr<Processor> processor : copy.m_processors) {
      for (AutoPtr<ProcessorInput> input : processor->inputs()) {
        if (input.isNull()) continue;
        if (input->m_link.isNull()) continue;

        AutoPtr<ProcessorOutput> output = input->m_link;

        Processor* old1 = input->owner();
        Processor* old2 = output->owner();

        auto it1 = std::find_if(proc_old_new.begin(), proc_old_new.end(),
          [old1](const std::pair<AutoPtr<Processor>, AutoPtr<Processor>>& element) { return element.first.raw() == old1; });
        auto it2 = std::find_if(proc_old_new.begin(), proc_old_new.end(),
          [old2](const std::pair<AutoPtr<Processor>, AutoPtr<Processor>>& element) { return element.first.raw() == old2; });

        if (it1 == proc_old_new.end() || it2 == proc_old_new.end()) continue;

        AutoPtr<Processor> new1 = it1->second;
        AutoPtr<Processor> new2 = it2->second;

        connect(new2->output(output->name()), new1->input(input->name()));
      }
    }

    // recreate GroupIO
    for (GroupInput ginput : copy.m_group_inputs) {
      auto it_ginput = std::find_if(input_old_new.begin(), input_old_new.end(),
        [ginput](const std::pair<AutoPtr<ProcessorInput>, AutoPtr<ProcessorInput>>& element) { return element.first == ginput.first; });
      if (it_ginput == input_old_new.end()) continue;
      
      AutoPtr<ProcessorInput> old_exposed_input = it_ginput->first;
      AutoPtr<ProcessorInput> new_exposed_input = it_ginput->second;
      
      Processor* old_groupprocessor = old_exposed_input->owner();
      auto it_proc = std::find_if(proc_old_new.begin(), proc_old_new.end(),
        [old_groupprocessor](const std::pair<AutoPtr<Processor>, AutoPtr<Processor>>& element) { return element.first.raw() == old_groupprocessor; });
      if (it_proc == proc_old_new.end()) continue;
      
      AutoPtr<Processor> new_groupprocessor = it_proc->second;

      AutoPtr<ProcessorOutput> new_groupinput = new_groupprocessor->output(new_exposed_input->name());

      m_group_inputs.push_back(GroupInput(new_exposed_input, new_groupinput));
    }
  }

  ProcessingGraph::~ProcessingGraph() {
    m_group_inputs.clear();
    m_group_outputs.clear();

    for (AutoPtr<Processor> processor : m_processors) {
      remove(processor);
    }
    for (AutoPtr<VisualComment> element : m_comments) {
      remove(element);
    }
    Processor::~Processor();
  }

  void ProcessingGraph::remove(AutoPtr<Processor>& _processor) {
    // disconnect
    if (_processor->owner() == this) {
      for (AutoPtr<ProcessorInput> input : _processor->inputs()) {
        if (!input.isNull()) {
          disconnect(input);
        }
      }
      for (AutoPtr<ProcessorOutput> output : _processor->outputs()) {
        if (!output.isNull()) {
          disconnect(output);
        }
      }
    }

    // remove the processor
    m_processors.erase(std::remove(m_processors.begin(), m_processors.end(), _processor), m_processors.end());
    //_processor.~AutoPtr();
  }

  void ProcessingGraph::remove(AutoPtr<VisualComment>& _comment) {
    m_comments.erase(std::remove(m_comments.begin(), m_comments.end(), _comment), m_comments.end());
  }

  void ProcessingGraph::remove(AutoPtr<SelectableUI>& _select) {
    AutoPtr<Processor> proc = AutoPtr<Processor>(_select);
    if (!proc.isNull()) {
      remove(proc);
    }
    AutoPtr<VisualComment> com = AutoPtr<VisualComment>(_select);
    if (!com.isNull()) {
      remove(com);
    }
  }

  bool ProcessingGraph::connect(const AutoPtr<Processor>& from, const std::string output_name, const AutoPtr<Processor>& to, const std::string input_name)
  {
    // check if the processors exists
    if (from.isNull() || to.isNull()) {
      return false;
    }

    AutoPtr<ProcessorOutput> output = from->output(output_name);
    AutoPtr<ProcessorInput> input = to->input(input_name);

    return connect(output, input);
  }

  bool ProcessingGraph::connect(AutoPtr<ProcessorOutput>& from, AutoPtr<ProcessorInput>& to)
  {
    // check if the processors i/o exists
    if (from.isNull() || to.isNull()) {
      return false;
    }

    // check if the processors comes from the same graph
    if (from->owner()->owner() != to->owner()->owner()) {
      return false;
    }

    // check if input already connected
    if (!to->m_link == NULL) {
      disconnect(to);
    }

    // if the new pipe create a cycle
    if (areConnected(to->owner(), from->owner())) {
      return false;
    }

    to->m_link = from;
    from->m_links.push_back(to);

    return true;
  }

  void ProcessingGraph::disconnect(AutoPtr<ProcessorInput>& to) {
    if (to.isNull()) return;

    AutoPtr<ProcessorOutput> from = to->m_link;
    if (!from.isNull()) {
      from->m_links.erase(std::remove(from->m_links.begin(), from->m_links.end(), to), from->m_links.end());
    }

    to->m_link = AutoPtr<ProcessorOutput>(NULL);
  }

  void ProcessingGraph::disconnect(AutoPtr<ProcessorOutput>& from) {
    if (from.isNull()) return;

    for (AutoPtr<ProcessorInput> to : from->m_links) {
      if (!to.isNull()) {
        to->m_link = AutoPtr<ProcessorOutput>(NULL);
      }
    }
    from->m_links.clear();
  }


  AutoPtr<ProcessingGraph> ProcessingGraph::collapseSubset(const std::vector<AutoPtr<SelectableUI>>& subset)
  {
    AutoPtr<ProcessingGraph> innerGraph   = addProcessor<ProcessingGraph>();
    AutoPtr<GroupProcessor>  groupInputs  = innerGraph->addProcessor<GroupProcessor>();
    AutoPtr<GroupProcessor>  groupOutputs = innerGraph->addProcessor<GroupProcessor>();

    groupInputs->setName("Group Input");
    groupInputs->setInputMode(true);
    groupOutputs->setName("Group Output");
    groupOutputs->setOutputMode(true);

    // move the entities
    for (AutoPtr<SelectableUI> select : subset) {
      innerGraph->add(select);
    }

    // edit border pipes
    for (AutoPtr<SelectableUI> select : subset) {
      AutoPtr<Processor> processor = AutoPtr<Processor>(select);
      if (processor.isNull()) {
        continue;
      }

      for (AutoPtr<ProcessorInput> input : processor->inputs()) {
        AutoPtr<ProcessorOutput> output = input->m_link;
        // not linked
        if (output.isNull()) continue;
        // same graph, nothing to do
        if (output->owner()->owner() == input->owner()->owner()) continue;

        auto it = std::find_if(innerGraph->m_group_inputs.begin(), innerGraph->m_group_inputs.end(),
          [output](const std::pair<AutoPtr<ProcessorInput>, AutoPtr<ProcessorOutput>>& element) { 
            return element.first->m_link.raw() == output.raw();
          });

        if (it != innerGraph->m_group_inputs.end()) {
          AutoPtr<ProcessorOutput> groupInput = it->second;
          connect(groupInput, input);
          continue;
        }

        AutoPtr<ProcessorInput> exposedInput = innerGraph->addInput(input->name(), input->type());
        AutoPtr<ProcessorOutput> groupInput = groupInputs->addOutput(exposedInput->name(), input->type());

        innerGraph->m_group_inputs.push_back(GroupInput(exposedInput, groupInput));

        disconnect(input);
        connect(output, exposedInput);
        connect(groupInput, input);
      }


      for (AutoPtr<ProcessorOutput> output : processor->outputs()) {
        bool linked = false;
        
        for (AutoPtr<ProcessorInput> input : output->m_links) {
          // not linked
          if (input.isNull()) continue;
          // same graph, nothing to do
          if (input->owner()->owner() == output->owner()->owner()) continue;

          linked = true;
          break;
        }

        if (linked) {

          std::string base = output->name();
          std::string name = base;
          int nb = 1;
          while (!innerGraph->output(name).isNull()) {
            name = base + "_" + std::to_string(nb++);
          }

          AutoPtr<ProcessorOutput> innerOutput = innerGraph->addOutput(name, output->type());
          AutoPtr<ProcessorInput> innerOutputGroup = groupOutputs->addInput(name, output->type());
          innerGraph->m_group_outputs.emplace_back(innerOutput, innerOutputGroup);

          for (AutoPtr<ProcessorInput> input : output->m_links) {
            if (input->owner()->owner() != output->owner()->owner()) {
              disconnect(input);
              connect(innerOutput, input);
            }
          }
          connect(output, innerOutputGroup);
        }
      }
    }

    for (AutoPtr<SelectableUI> select : subset) {
      remove(select);
    }

    AABox bbox = getBoundingBox(subset);
    ImVec2 barycenter = getBarycenter(subset);

    innerGraph->setPosition(barycenter);
    groupInputs->setPosition (ImVec2(bbox.minCorner()[0] - style.processor_width * 2, barycenter.y));
    groupOutputs->setPosition(ImVec2(bbox.maxCorner()[0] + style.processor_width * 2, barycenter.y));
    return innerGraph;
  }

  void ProcessingGraph::expandGraph(AutoPtr<ProcessingGraph>& collapsed, ImVec2 position) {

    // Move processors
    for (AutoPtr<Processor> processor : collapsed->m_processors) {
      AutoPtr<GroupProcessor> proc(processor);
      // If the processor is not a GroupProcessor
      if (proc.isNull()) {
        addProcessor(processor);
      }
    }

    for (AutoPtr<VisualComment> comment : collapsed->m_comments) {
        addComment(comment);
    }
    
    // Update pipes
    for (GroupInput gi : collapsed->m_group_inputs) {
      AutoPtr<ProcessorOutput> output = gi.first->m_link;
      for (AutoPtr<ProcessorInput> input : gi.second->m_links) {
        connect(output, input);
      }
    }
    for (GroupOutput go : collapsed->m_group_outputs) {
      AutoPtr<ProcessorOutput> output = go.second->m_link;
      for (AutoPtr<ProcessorInput> input : go.first->m_links) {
        connect(output, input);
      }
    }

    ImVec2 offset = position - collapsed->getBarycenter();
    for (AutoPtr<Processor> processor : collapsed->m_processors) {
      processor->setPosition(processor->getPosition() + offset);
    }

    remove(AutoPtr<Processor>(collapsed));
  }

  AutoPtr<ProcessingGraph> ProcessingGraph::copySubset(const std::vector<AutoPtr<SelectableUI>>& subset)
  {
    AutoPtr<ProcessingGraph> graph = AutoPtr<ProcessingGraph>(new ProcessingGraph());
    
    std::vector<std::pair<AutoPtr<Processor>, AutoPtr<Processor>>> assoc;
    
    // copy the node
    for (AutoPtr<SelectableUI> select : subset) {
      AutoPtr<SelectableUI> new_select = select->clone();
      new_select->setOwner(nullptr);
      new_select->setPosition(select->getPosition());

      AutoPtr<Processor> processor = AutoPtr<Processor>(select);
      AutoPtr<Processor> new_processor = AutoPtr<Processor>(new_select);
      if (!processor.isNull() && !new_processor.isNull()) {
        assoc.push_back(std::make_pair(processor, new_processor));
      }
      graph->add(new_select);
    }
    // recreate the pipes
    for (AutoPtr<SelectableUI> select : subset) {
      AutoPtr<Processor> processor = AutoPtr<Processor>(select);
      if (processor.isNull()) {
        continue;
      }
      for (AutoPtr<ProcessorInput> input : processor->inputs()) {
        if (input.isNull()) continue;
        if (input->m_link.isNull()) continue;

        AutoPtr<ProcessorOutput> output = input->m_link;

        Processor* old1 = input->owner();
        Processor* old2 = output->owner();

        auto it1 = std::find_if(assoc.begin(), assoc.end(),
          [old1](const std::pair<AutoPtr<Processor>, AutoPtr<Processor>>& element) { return element.first.raw() == old1; });
        auto it2 = std::find_if(assoc.begin(), assoc.end(),
          [old2](const std::pair<AutoPtr<Processor>, AutoPtr<Processor>>& element) { return element.first.raw() == old2; });

        if (it1 == assoc.end() || it2 == assoc.end()) continue;

        AutoPtr<Processor> new1 = it1->second;
        AutoPtr<Processor> new2 = it2->second;
        
        connect(new2->output(output->name()), new1->input(input->name()));
      }
    }

    return graph;
  }

  bool ProcessingGraph::areConnected(Processor * from, Processor * to)
  {
    // Raw pointers, because m_owner is a raw pointer
    std::queue<Processor*> toCheck;
    toCheck.push(to);

    while (!toCheck.empty()) {
      Processor* current = toCheck.front();
      toCheck.pop();

      if (current == from) {
        return true;
      }
      for (AutoPtr<ProcessorInput> input : current->inputs()) {
        if (!input->m_link.isNull()) {
          AutoPtr<ProcessorOutput> output = input->m_link;
          toCheck.push(output->owner());
        }
      }
    }
    return false;
  }

  void ProcessingGraph::save(std::ofstream& _stream) {
    _stream << "p_" << getUniqueID() << " = Graph('" << name() << "')" << std::endl;
    
    ImVec2 bar = getBarycenter();
    // Save the nodes
    for (AutoPtr<Processor> proc : m_processors) {
      proc->translate(ImVec2(0,0)-bar);
      proc->save(_stream);
      proc->translate(bar);
      _stream << "p_" << getUniqueID() << ":add( p_" << proc->getUniqueID() << ")" << std::endl;
    }

    // Save the connections
    for (AutoPtr<Processor> proc : m_processors) {
      for (AutoPtr<ProcessorInput> input : proc->inputs()) {
        AutoPtr<ProcessorOutput> output = input->m_link;
        if (output.isNull()) continue;
        _stream << "connect( o_" << output->getUniqueID() << ", i_" << input->getUniqueID() << ")" << std::endl;
      }
    }
    _stream << "set_graph(p_" << getUniqueID() << ")" << std::endl;
  }

  void ProcessingGraph::iceSL(std::ofstream& _stream) {
    std::set<Processor*> done;
    std::unordered_set<Processor*> toDo;

    for (AutoPtr<Processor> processor : m_processors) {
      bool not_connected = true;
      for (AutoPtr<ProcessorInput> input : processor->inputs()) {
        if (!input.isNull() && !input->m_link.isNull()) {
          not_connected = false;
          break;
        }
      }
      if (not_connected) {
        toDo.emplace(processor.raw());
      }
    }

    _stream << "--[[ " + std::string(name()) + " ]]--" << std::endl <<
               "setfenv(1, _G0)  --go back to global initialization" << std::endl <<
               "__currentNodeId = " << std::to_string((int64_t)this) << std::endl;

    if ( (owner() != NULL && owner()->isDirty()) || isDirty() || isEmiter()) {
      _stream << "setDirty(__currentNodeId)" << std::endl;
    }

    _stream << "if (isDirty({__currentNodeId";

    for (auto input : inputs()) {
      if (!input->m_link.isNull()) {
        std::string s2 = std::to_string((int64_t)input->m_link->owner());
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

      for (AutoPtr<ProcessorOutput> output : processor->outputs()) {
        for (AutoPtr<ProcessorInput> nextInput : output->m_links) {
          bool all_inputs_done = true;
          for (AutoPtr<ProcessorInput> input : nextInput->owner()->inputs()) {
            if (!input.isNull() && !input->m_link.isNull()) {
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