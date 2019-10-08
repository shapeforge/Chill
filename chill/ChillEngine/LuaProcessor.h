#pragma once

#include "Processor.h"

#include <tuple>

namespace chill
{
  /**
   *
   */
  class LuaProcessor : public Processor
  {
  private:
    fs::path    m_nodepath;
    std::string m_program;
    bool        m_program_edited = false;

    std::tuple<int, int> m_icesl_export_linenumbers;

    LuaProcessor(LuaProcessor &_processor);
  public:
    LuaProcessor(const fs::path &_path);

    std::shared_ptr<SelectableUI> clone() override {
      return std::shared_ptr<SelectableUI>(new LuaProcessor(*this));
    }

    void save(std::ofstream& _stream) override;
    void iceSL(std::ofstream& stream) override;

    void ParseInput();
    void ParseOutput();
    void ParseOptional();

    void Parse() {
      ParseInput();
      ParseOutput();
      ParseOptional();

      /*if (outputs().empty()) {
        // No input nor output, should be an old node or object
        if (inputs().empty()) {
          // TODO: transform emit into shape output
        }
        // No output, should be an emitter
        else {
          m_emit = true;
        }
      }*/
    }


    /**
    *  Add a new input to the processor.
    *  @param _input The input.
    *  @return A pointer to the new input.
    */
    std::shared_ptr<ProcessorInput> addInput(std::shared_ptr<ProcessorInput> _input) override;

    /**
    *  Add a new output to the processor.
    *  @param _output The output.
    *  @return A pointer to the new output.
    **/
    std::shared_ptr<ProcessorOutput> addOutput(std::shared_ptr<ProcessorOutput> _output)  override;

    /**
    *  Add a new output to the processor.
    *  @param _name The output name.
    *  @param _type The output type.
    *  @return A pointer to the new output.
    **/
    std::shared_ptr<ProcessorOutput> addOutput(std::string _name, IOType::IOType _type = IOType::UNDEF, bool _emitable = false)
    {
      return addOutput(ProcessorOutput::create(_name, _type, _emitable));
    }

    /**
    *  Add a new input to the processor.
    *  @param _name The input name.
    *  @param _type The input type.
    *  @param _args All additionnal arguments
    *  @return A pointer to the new input.
    */
    template <typename ... Args>
    std::shared_ptr<ProcessorInput> addInput(std::string _name, IOType::IOType _type, Args&& ... _args)
    {
      return addInput(ProcessorInput::create(_name, _type, _args...));
    }

  };
}
