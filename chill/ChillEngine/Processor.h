/** @file */
#pragma once

//-------------------------------------------------------

#include <memory>
#include <iostream>

#include "IOs.h"
#include "IOTypes.h"
#include "UI.h"

//-------------------------------------------------------
// ProcessingGraph.h
namespace chill {
  class ProcessingGraph;
}

//-------------------------------------------------------

namespace chill {
  class Processor;
  class LuaProcessor;
  class GroupProcessor;
}


//-------------------------------------------------------
namespace chill {
  enum ProcessorState {
    DEFAULT,
    DISABLED,
    EMITING
  };
  

  /**
   *  Processor class.
   *  Is a node within the graph. Contains inputs and outputs.
   **/
  class Processor : public SelectableUI
  {
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
     **/
    const std::vector<std::shared_ptr<ProcessorInput>> inputs() {
      return m_inputs;
    }

    /**
     *  Get the list of outputs.
     *  @return The list of outputs.
     **/
    std::vector<std::shared_ptr<ProcessorOutput>> outputs() {
      return m_outputs;
    }

    /**
     *  Get an input by name.
     *  @param _name The input name.
     *  @return The input pointer if exists else a null pointer.
     **/
    std::shared_ptr<ProcessorInput> input(std::string _name);

    /**
     *  Get an output by name.
     *  @param _name The output name.
     *  @return The output pointer if exists else a null pointer.
     **/
    std::shared_ptr<ProcessorOutput> output(std::string _name);

    /**
    *  Add a new input to the processor.
    *  @param _input The input.
    *  @return A pointer to the new input.
    **/
    virtual std::shared_ptr<ProcessorInput> addInput(std::shared_ptr<ProcessorInput> _input);

    /**
     *  Add a new input to the processor.
     *  @param _name The input name.
     *  @param _type The input type.
     *  @param _args All additionnal arguments
     *  @return A pointer to the new input.
     **/
    template <typename ... Args>
    std::shared_ptr<ProcessorInput> addInput(std::string _name, IOType::IOType _type, Args&& ... _args) {
      return addInput(ProcessorInput::create(_name, _type, _args...));
    }

    /**
     *  Add a new output to the processor.
     *  @param _output The output.
     *  @return A pointer to the new output.
     **/
    virtual std::shared_ptr<ProcessorOutput> addOutput(std::shared_ptr<ProcessorOutput> _output);

    /**
     *  Add a new output to the processor.
     *  @param _name The output name.
     *  @param _type The output type.
     *  @return A pointer to the new output.
     **/
    std::shared_ptr<ProcessorOutput> addOutput(std::string _name, IOType::IOType _type = IOType::UNDEF, bool _emitable = false);

    /**
     *  Remove an input.
     *  @param _input The pointer that refers to the input
     **/
    void removeInput(std::shared_ptr<ProcessorInput>& _input);

    /**
     *  Remove an output.
     *  @param _output The pointer that refers to the output
     **/
    void removeOutput(std::shared_ptr<ProcessorOutput>& _output);


    void replaceInput(std::string _inputName, std::shared_ptr<ProcessorInput> _input) {
      std::replace(m_inputs.begin(), m_inputs.end(), input(_inputName), _input);
    }

    void replaceOutput(std::string _outputName, std::shared_ptr<ProcessorOutput> _output) {
      std::replace(m_outputs.begin(), m_outputs.end(), output(_outputName), _output);
    }

    /**
     *  Add a new connection to the graph if, and only if, there is no cycle created.
     *  @param _from The origin processor.
     *  @param _output_name The name of the output.
     *  @param _to The destination processor.
     *  @param _input_name The name of the input.
     **/
    static bool connect(std::shared_ptr<Processor> _from, const std::string _output_name,
      std::shared_ptr<Processor> _to, const std::string _input_name);

    /**
     *  Add a new connection to the graph if, and only if, there is no cycle created.
     *  @param _from The processor's output.
     *  @param _to The processor's input.
     **/
    static bool connect(std::shared_ptr<ProcessorOutput> _from, std::shared_ptr<ProcessorInput> _to);

    /**
     *  Remove a connection in the graph.
     *  @param _to The processor's input.
     **/
    static void disconnect(std::shared_ptr<ProcessorInput> _to);

    /**
     *  Remove a connection in the graph.
     *  @param _from The processor's output.
     **/
    static void disconnect(std::shared_ptr<ProcessorOutput> _from);

    /**
     *  Check if two processors are connected.
     *  @param _from The first processor.
     *  @param _to The second processor.
     *  @return true if they are connected.
     **/
    static bool areConnected(Processor * _from, Processor * _to);

    /**
     *  Draw the processor
     **/
    virtual bool draw();

    /**
     *  Make a copy of the processor (deep copy).
     **/
    virtual std::shared_ptr<SelectableUI> clone() {
      return std::shared_ptr<SelectableUI>(new Processor(*this));
    }

    /**
     *  Generate the lua code to recreate this processor and add it to the stream.
     *  @param _stream The output stream.
     **/
    virtual void save(std::ofstream& _stream);

    /**
     *  Generate the IceSL lua code and add it to the stream.
     *  @param _stream The output stream.
     **/
    virtual void iceSL(std::ofstream& _stream);

    void setEmiter(bool _emit = true) {
      m_emit = _emit;
    }

    /**
     * @return true if affects any output (shape or slicing parameter)
     **/
    inline bool isEmiter() {
      return m_emit;
    }

    inline void setDirty(bool _dirty = true) {
      m_dirty = _dirty;
    }

    /**
     * @return true if it needs to be refreshed
     **/
    inline bool isDirty() {
      return m_dirty;
    }

    ProcessorState getState() {
      return m_state;
    }

  private:
    /** Emit a shape or a slicing parameter */
    bool                                  m_emit = false;


    ProcessorState                        m_state = DEFAULT;
    /** List of all inputs. */
    std::vector<std::shared_ptr<ProcessorInput>>  m_inputs;
    /** List of all outputs. */
    std::vector<std::shared_ptr<ProcessorOutput>> m_outputs;
    /** Next nodes have to update themselves. */
    bool                                  m_dirty = true;

  };

  //-------------------------------------------------------

  class GroupProcessor : public Processor
  {
  public:
    GroupProcessor();

    virtual std::shared_ptr<SelectableUI> clone() override {
      return std::shared_ptr<SelectableUI>(new GroupProcessor(*this));
    }

    bool draw() override;

    void setInputMode(bool mode_) {
      m_is_input = mode_;
    }

    void setOutputMode(bool mode_) {
      m_is_output = mode_;
    }

    void iceSL(std::ofstream& _stream) override;

  protected:
    bool m_is_input  = false;
    bool m_is_output = false;
  };

  //-------------------------------------------------------

  class Multiplexer : public Processor
  {
  public:
    Multiplexer();

    virtual std::shared_ptr<SelectableUI> clone() override {
      return std::shared_ptr<SelectableUI>(new Multiplexer(*this));
    }

    bool draw() override;

    void iceSL(std::ofstream& _stream) override;
  };
}
