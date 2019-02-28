/** @file */
#pragma once

// IOs.h
namespace Chill
{
  class ProcessorInput;
  class ProcessorOutput;
};

// ProcessingGraph.h
namespace Chill
{
  class ProcessingGraph;
}

namespace Chill
{
  class Processor;
  class LuaProcessor;
  class GroupProcessor;
};

#include <LibSL.h>

#include "UI.h"
#include "IOTypes.h"
#include "IOs.h"


namespace Chill
{
  /**
   *  Processor class.
   *  Is a node within the graph. Contains inputs and outputs.
   **/
  class Processor : public SelectableUI
  {
  private:
    /** Emit a shape or a slicing parameter */
    bool                                  m_emit = false;
    /** List of all inputs. */
    std::vector<AutoPtr<ProcessorInput>>  m_inputs;
    /** List of all outputs. */
    std::vector<AutoPtr<ProcessorOutput>> m_outputs;
    /** Next nodes have to update themselves. */
    bool                                  m_dirty = false;

  public:
    /**
     *  Instanciate a new Processor.
     **/
    Processor()
    {
      m_name = "Processor";
      m_color = style.processor_default_color;
    }

    /**
     *  Instanciate a new Processor.
     *  @param _name The name of the processor
     **/
    Processor(const std::string& _name)
    {
      m_name = _name;
      m_color = style.processor_default_color;
    }

    /**
     *  Instanciate a new Processor and set is owner.
     *  @param _owner Parent graph that contains the processor.
     **/
    Processor(ProcessingGraph * _owner)  
    {
      m_name  = "Processor";
      m_owner = _owner;
      m_color = style.processor_default_color;
    }

    /**
     *  Copy constructor.
     *  @param _copy The processor which gonna be copied.
     **/
    Processor(Processor &_copy);

    virtual ~Processor();

    /**
     *  Get the list of inputs.
     *  @return The list of inputs.
     */
    const std::vector<AutoPtr<ProcessorInput>> inputs() {
      return m_inputs;
    }

    /**
     *  Get the list of outputs.
     *  @return The list of outputs.
     */
    std::vector<AutoPtr<ProcessorOutput>> outputs() {
      return m_outputs;
    }

    /**
     *  Get an input by name.
     *  @param _name The input name.
     *  @return The input pointer if exists else a null pointer.
     */
    AutoPtr<ProcessorInput> input(std::string _name);

    /**
     *  Get an output by name.
     *  @param _name The output name.
     *  @return The output pointer if exists else a null pointer.
     */
    AutoPtr<ProcessorOutput> output(std::string _name);

    /**
    *  Add a new input to the processor.
    *  @param _input The input.
    *  @return A pointer to the new input.
    */
    virtual AutoPtr<ProcessorInput> addInput(AutoPtr<ProcessorInput> _input);

    /**
     *  Add a new input to the processor.
     *  @param _name The input name.
     *  @param _type The input type.
     *  @param _args All additionnal arguments
     *  @return A pointer to the new input.
     */
    template <typename ... Args>
    AutoPtr<ProcessorInput> addInput(std::string _name, IOType::IOType _type, Args&& ... _args);

    /**
     *  Add a new output to the processor.
     *  @param _output The output.
     *  @return A pointer to the new output.
     **/
    virtual AutoPtr<ProcessorOutput> addOutput(AutoPtr<ProcessorOutput> _output);

    /**
     *  Add a new output to the processor.
     *  @param _name The output name.
     *  @param _type The output type.
     *  @return A pointer to the new output.
     **/
    AutoPtr<ProcessorOutput> addOutput(std::string _name, IOType::IOType _type = IOType::UNDEF);

    /**
     *  Remove an input.
     *  @param _input The pointer that refers to the input
     **/
    void removeInput(AutoPtr<ProcessorInput>& _input);

    /**
     *  Remove an output.
     *  @param _output The pointer that refers to the output
     **/
    void removeOutput(AutoPtr<ProcessorOutput>& _output);


    void replaceInput(std::string _inputName, AutoPtr<ProcessorInput> _input) {
      std::replace(m_inputs.begin(), m_inputs.end(), input(_inputName), _input);
    }

    void replaceOutput(std::string _outputName, AutoPtr<ProcessorOutput> _output) {
      std::replace(m_outputs.begin(), m_outputs.end(), output(_outputName), _output);
    }


    /**
     *  Draw the processor
     **/
    virtual bool draw();

    /**
     *  Make a copy of the processor (deep copy)
     **/
    virtual AutoPtr<SelectableUI> Processor::clone() {
      return AutoPtr<SelectableUI>(new Processor(*this));
    };

    /**
     *  Generate the lua code and add it to the stream
     *  @param _stream The output stream
     **/
    virtual void save(std::ofstream& _stream);

    virtual void iceSL(std::ofstream& _stream);

    void setEmiter(bool _emit = true) {
      m_emit = _emit;
    }

    /**
     * @return true if affects any output (shape or slicing parameter)
     */
    virtual bool isEmiter();


    void setDirty(bool _dirty = true) {
      m_dirty = _dirty;
    }

    virtual bool isDirty() {
      return m_dirty;
    }
  };





  class GroupProcessor : public Processor
  {
    bool m_is_input = false;
    bool m_is_output = false;
    //GroupProcessor(const GroupProcessor &processor);
  public:
    GroupProcessor();

    virtual AutoPtr<SelectableUI> clone() override {
      return AutoPtr<SelectableUI>(new GroupProcessor(*this));
    }

    virtual bool draw();

    void setInputMode(bool mode_) {
      m_is_input = mode_;
    }

    void setOutputMode(bool mode_) {
      m_is_output = mode_;
    }

    virtual void iceSL(std::ofstream& _stream);
  };


  class Multiplexer : public Processor
  {
  public:
    Multiplexer();

    virtual AutoPtr<SelectableUI> clone() override {
      return AutoPtr<SelectableUI>(new Multiplexer(*this));
    }

    bool draw();

    virtual void iceSL(std::ofstream& _stream);
  };
};


