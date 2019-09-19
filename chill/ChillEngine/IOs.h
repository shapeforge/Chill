/** @file */
#pragma once

#include <regex>

#include "imgui/imgui.h"

#include "UI.h"
#include "IOTypes.h"

// COLOR BLIND FRIENDLY PALETTE
static ImColor color_undef (255, 255, 255);
static ImColor color_bool  (146, 0  , 0  );
static ImColor color_number(182, 109, 255);
static ImColor color_string(109, 182, 255);
static ImColor color_shape (36 , 255, 36 );
static ImColor color_vec4  (255, 255, 109);
static ImColor color_vec3  (219, 209, 0  );

// -----------------------------------------------------

namespace chill {
class Processor;
class ProcessorInput;
class ProcessorOutput;
}

//-------------------------------------------------------

namespace chill {

class IO : public UI {
  public:
    inline Processor * owner() {
      return m_owner;
    }

    //-------------------------------------------------------


    void setOwner(Processor * owner) {
      m_owner = owner;
    }

    //-------------------------------------------------------

    inline const char * name() {
      return m_name.c_str();
    }

    //-------------------------------------------------------

    void setName(std::string name) {
      m_name = name;
    }

    //-------------------------------------------------------

    inline IOType::IOType type() {
      return m_type;
    }

    //-------------------------------------------------------

    void setType(IOType::IOType type) {
      m_type = type;
    }

    //-------------------------------------------------------

    inline ImU32 color() {
      return m_color;
    }

    //-------------------------------------------------------

    void setColor(ImU32 color) {
      m_color = color;
    }

    //-------------------------------------------------------

  private:
    /** Parent processor, raw pointer is needed. */
    Processor *    m_owner;
    /** Display name. */
    std::string    m_name;
    /** Expected data type. */
    IOType::IOType m_type;
    /** Display color. */
    ImU32          m_color;
}; // class IO

//-------------------------------------------------------

/**
*  OutputProcessor class.
*  Represents a node's output socket.
*/
class ProcessorOutput : public IO {
  public:
    static std::shared_ptr<ProcessorOutput> create(const std::string& name, IOType::IOType type, bool emitable);

    //-------------------------------------------------------

    virtual std::shared_ptr<ProcessorOutput> clone() = 0;

    //-------------------------------------------------------

    ~ProcessorOutput();

    //-------------------------------------------------------

    void setEmitable(bool _emit) {
      m_emitable = _emit;
    }

    //-------------------------------------------------------

    bool isEmitable() {
      return m_emitable;
    }

    //-------------------------------------------------------

    bool draw();

    //-------------------------------------------------------

    virtual void save(std::ofstream& _stream) {
      _stream << "o_" << getUniqueID() << " = Output({" <<
                 "name = '" << name() << "', " <<
                 "type = '" << IOType::ToString(type()) << "'" <<
                 "})" << std::endl;
    }

    //-------------------------------------------------------

    /** List of all linked inputs. */
    std::vector<std::shared_ptr<ProcessorInput>> m_links;
    /** contains emitable data */
    bool m_emitable = false;
}; // class ProcessorOutput

//-------------------------------------------------------

/**
*  InputProcessor class.
*  Represents a node's input socket.
*/
class ProcessorInput : public IO {
  public:
    template <typename ... Args>
    static std::shared_ptr<ProcessorInput> create(const std::string& _name, IOType::IOType _type, Args&& ... _args);

    //-------------------------------------------------------

    virtual std::shared_ptr<ProcessorInput> clone() = 0;

    //-------------------------------------------------------

    ~ProcessorInput();

    //-------------------------------------------------------

    bool draw();

    //-------------------------------------------------------

    virtual bool drawTweak() = 0;

    //-------------------------------------------------------

    virtual void save(std::ofstream& _stream) {
      _stream << "i_" << getUniqueID() << " = Input({" <<
                 "name = '" << name() << "', " <<
                 "type = '" << IOType::ToString(type()) << "'" <<
                 "})" << std::endl;
    }

    //-------------------------------------------------------

    virtual std::string getLuaValue();

    //-------------------------------------------------------

    /** Linked output. */
    std::shared_ptr<ProcessorOutput> m_link;

    /** Is not linkable */
    bool m_isDataOnly = false;

}; // class ProcessorInput

//-------------------------------------------------------

// IO_UNDEF
class UndefInput : public ProcessorInput
{
  public:
    UndefInput() {
      setType(IOType::UNDEF);
      setColor(color_undef);
    }

    //-------------------------------------------------------

    std::shared_ptr<ProcessorInput> clone() {
      std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new UndefInput());
      input->setName (name());
      input->setColor(color());
      input->m_isDataOnly = m_isDataOnly;
      return input;
    }

    //-------------------------------------------------------

    // For compatibility (shouldn't be called)
    template <typename ...>
    UndefInput(...) : UndefInput() {}

    //-------------------------------------------------------

    bool drawTweak();
};

//-------------------------------------------------------

class UndefOutput : public ProcessorOutput
{
  public:
    std::shared_ptr<ProcessorOutput> clone() {
      std::shared_ptr<ProcessorOutput> output = std::shared_ptr<ProcessorOutput>(new UndefOutput());
      output->setName (name());
      output->setColor(color());
      return output;
    }

    //-------------------------------------------------------

    UndefOutput() {
      setType(IOType::UNDEF);
      setColor(color_undef);
    }
};

//-------------------------------------------------------

// IO_IMPLICT
class ImplicitInput : public ProcessorInput
{
public:
  ImplicitInput() {
    setType(IOType::IMPLICIT);
    setColor(color_undef);
  }

  //-------------------------------------------------------

  std::shared_ptr<ProcessorInput> clone() {
    std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new ImplicitInput());
    input->setName(name());
    input->setColor(color());
    input->m_isDataOnly = m_isDataOnly;
    return input;
  }

  //-------------------------------------------------------

  // For compatibility (shouldn't be called)
  template <typename ...>
  ImplicitInput(...) : ImplicitInput() {}

  //-------------------------------------------------------

  bool drawTweak();
};

//-------------------------------------------------------

class ImplicitOutput : public ProcessorOutput
{
public:
  std::shared_ptr<ProcessorOutput> clone() {
    std::shared_ptr<ProcessorOutput> output = std::shared_ptr<ProcessorOutput>(new ImplicitOutput());
    output->setName(name());
    output->setColor(color());
    return output;
  }

  //-------------------------------------------------------

  ImplicitOutput() {
    setType(IOType::IMPLICIT);
    setColor(color_undef);
  }
};

//-------------------------------------------------------

// IO_BOOL
class BoolInput : public ProcessorInput
{
  public:
    BoolInput() {
      setType (IOType::BOOLEAN);
      setColor(color_bool);
    }

    //-------------------------------------------------------

    template <typename ...>
    BoolInput(bool _value = false, ...) : BoolInput() {
      m_value = _value;
    }

    //-------------------------------------------------------

    template <typename ...>
    BoolInput(std::vector<std::string> _params) : BoolInput() {
      size_t s = _params.size();
      m_value = s >= 1 ? _params[0] == "true": false;
    }

    template <typename ...>
    BoolInput(...) : BoolInput() {
      sl_assert(false);
    }

    std::shared_ptr<ProcessorInput> clone() {
      std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new BoolInput(m_value));
      input->setName (name());
      input->setColor(color());
      input->m_isDataOnly = m_isDataOnly;
      return input;
    }

    bool drawTweak();

    void save(std::ofstream& _stream) {
      _stream << "i_" << getUniqueID() << " = Input({" <<
                 "name = '" << name() << "', " <<
                 "type = '" << IOType::ToString(type()) << "', " <<
                 "value = " << (m_value ? "true" : "false") <<
                 "})" << std::endl;
    }

    std::string getLuaValue() {
      return (m_value ? "true" : "false");
    }

    bool m_value;

};

//-------------------------------------------------------

class BoolOutput : public ProcessorOutput {
  public:
    BoolOutput()
    {
      setType (IOType::BOOLEAN);
      setColor(color_bool);
    }

    //-------------------------------------------------------

    std::shared_ptr<ProcessorOutput> clone() {
      std::shared_ptr<ProcessorOutput> output = std::shared_ptr<ProcessorOutput>(new BoolOutput());
      output->setName (name());
      output->setColor(color());
      return output;
    }
};

//-------------------------------------------------------

// IO_INT
class IntInput : public ProcessorInput {
  public:
    IntInput() {
      setType(IOType::INTEGER);
      setColor(color_number);
    }

    //-------------------------------------------------------

    template <typename ...>
    IntInput(int _value, int _min = min(), int _max = max(), bool _alt = false, int _step = step(), ...) : IntInput()
    {
      m_value = _value;
      m_min   = _min;
      m_max   = _max;
      m_alt   = _alt;
      m_step  = _step;

      m_value = std::min(m_max, std::max(m_min, m_value));

      if (m_step == 0) {
        if (m_min != min() && m_max != max()) {
          m_step = (m_max - m_min) / 100;
        }
        else {
          m_step = step();
        }
      }
    }

    //-------------------------------------------------------

    IntInput(std::vector<std::string>& _params) : IntInput() {
      size_t s = _params.size();

      m_value = s >= 1 ? std::stoi(_params[0]) : 0;
      m_min   = s >= 2 ? std::stoi(_params[1]) : min();
      m_max   = s >= 3 ? std::stoi(_params[2]) : max();
      m_alt   = s >= 4 ? _params[3] == "true"  : false;
      m_step  = s >= 5 ? std::stoi(_params[4]) : step();

      m_value = std::min(m_max, std::max(m_min, m_value));
    }

    //-------------------------------------------------------

    // For compatibility (shouldn't be called)
    template <typename ...>
    IntInput(...) {
      sl_assert(false);
    }

    //-------------------------------------------------------

    std::shared_ptr<ProcessorInput> clone() {
      std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new IntInput(m_value, m_min, m_max, m_alt, m_step));
      input->setName (name());
      input->setColor(color());
      input->m_isDataOnly = m_isDataOnly;
      return input;
    }

    //-------------------------------------------------------

    bool drawTweak();

    //-------------------------------------------------------

    void save(std::ofstream& _stream) {
      _stream << "i_" << getUniqueID() << " = Input({" <<
                 "name = '" << name() << "'" <<
                 ", type = '" << IOType::ToString(type()) << "'" <<
                 ", value = " << m_value <<
                 (m_min != min() ? ", min = " + std::to_string(m_min) : "") <<
                 (m_max != max() ? ", max = " + std::to_string(m_max) : "") <<
                 (m_alt ? ", alt = true" : "") <<
                 ", step = " + std::to_string(m_step) <<
                 "})" << std::endl;
    }

    //-------------------------------------------------------

    inline std::string getLuaValue() {
      return std::to_string(m_value);
    }

    //-------------------------------------------------------

    static inline int min() { return std::numeric_limits<int>().min(); }
    static inline int max() { return std::numeric_limits<int>().max(); }
    static inline int step() { return 1; }

    //-------------------------------------------------------

    int  m_value;
    int  m_min;
    int  m_max;
    bool m_alt;
    int  m_step;
};

//-------------------------------------------------------

class IntOutput : public ProcessorOutput
{
  public:
    IntOutput() {
      setType (IOType::INTEGER);
      setColor(color_number);
    }

    //-------------------------------------------------------

    std::shared_ptr<ProcessorOutput> clone() {
      std::shared_ptr<ProcessorOutput> output = std::shared_ptr<ProcessorOutput>(new IntOutput());
      output->setName (name());
      output->setColor(color());
      return output;
    }
};

//-------------------------------------------------------

// IO_LIST
class ListInput : public ProcessorInput {
  public:
    ListInput() {
      setType(IOType::INTEGER);
      setColor(color_number);
    }

    //-------------------------------------------------------

    template <typename ...>
    ListInput(int _value, std::vector<std::string>& _params) : ListInput() {
      int s = static_cast<int>(_params.size());

      m_value = _value;
      m_min = 0;
      m_max = s;

      m_value = std::min(m_max, std::max(m_min, m_value));
    }

    //-------------------------------------------------------

    template <typename ...>
    ListInput(std::vector<std::string>& _params) : ListInput() {
      int s = static_cast<int>(_params.size());

      m_values = _params;
      m_value  = 0;
      m_min    = 0;
      m_max    = s;

      m_value = std::min(m_max, std::max(m_min, m_value));
    }

    //-------------------------------------------------------

    // For compatibility (shouldn't be called)
    template <typename ...>
    ListInput(...) {
      sl_assert(false);
    }

    //-------------------------------------------------------

    std::shared_ptr<ProcessorInput> clone() {
      std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new ListInput(m_value, m_values));
      input->setName(name());
      input->setColor(color());
      input->m_isDataOnly = m_isDataOnly;
      return input;
    }

    //-------------------------------------------------------

    bool drawTweak();

    //-------------------------------------------------------

    void save(std::ofstream& _stream) {
      _stream << "i_" << getUniqueID() << " = Input({" <<
                 "name = '" << name() << "'" <<
                 ", type = '" << IOType::ToString(type()) << "'" <<
                 ", value = " << m_value <<
                 "})" << std::endl;
    }

    std::string getLuaValue() {
      return std::to_string(m_value);
    }

    //-------------------------------------------------------

    static inline int min() { return std::numeric_limits<int>().min(); }
    static inline int max() { return std::numeric_limits<int>().max(); }
    static inline int step() { return 1; }

    //-------------------------------------------------------

    std::vector<std::string> m_values;
    int  m_value;
    int  m_min;
    int  m_max;
};

//-------------------------------------------------------


// IO_PATH
class PathInput : public ProcessorInput {
  public:

    PathInput() {
      setType(IOType::STRING);
      setColor(color_string);
    }

    //-------------------------------------------------------

    // For compatibility (shouldn't be called)
    template <typename ...>
    PathInput(...) : PathInput() {
      sl_assert(false);
    }

    //-------------------------------------------------------

    template <typename ...>
    PathInput(std::string _value = "", bool _alt = false, std::string _filter = "", ...) : PathInput() {
      m_value  = _value;
      m_alt    = _alt;
      m_filter = _filter;
    }

    //-------------------------------------------------------

    template <typename ...>
    PathInput(std::vector<std::string>& _params) : PathInput() {
      size_t s = _params.size();

      m_value  = s >= 1 ? _params[0] : "";
      m_alt    = s >= 2 ? _params[1] == "true" : false;
      m_filter = s >= 3 ? _params[2] : "";

      std::regex quote("(^[\'\"]|[\'\"]$)");
      std::string unquoted;

      regex_replace(std::back_inserter(unquoted), m_value.begin(), m_value.end(), quote, "$2");
      m_value = unquoted;

      regex_replace(std::back_inserter(unquoted), m_filter.begin(), m_filter.end(), quote, "$2");
      m_filter = unquoted;
    }

    //-------------------------------------------------------

    std::shared_ptr<ProcessorInput> clone() {
      std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new PathInput(m_value));
      input->setName(name());
      input->setColor(color());
      input->m_isDataOnly = m_isDataOnly;
      return input;
    }

    //-------------------------------------------------------

    bool drawTweak();

    //-------------------------------------------------------

    void save(std::ofstream& _stream) {
      _stream << "i_" << getUniqueID() << " = Input({" <<
                 "name = '" << name() << "', " <<
                 "type = '" << IOType::ToString(type()) << "', " <<
                 "value = '" << m_value << "'" <<
                 (m_alt ? ", alt = true" : "") <<
                 "})" << std::endl;
    }

    //-------------------------------------------------------

    std::string getLuaValue() {
      std::regex escaped_chars("([\\[\\]\\{\\}\\\"\\\'])");
      std::string escaped;
      regex_replace(std::back_inserter(escaped), m_value.begin(), m_value.end(), escaped_chars, "\\$1");
      return "'" + escaped + "'";
    }

    //-------------------------------------------------------

    std::string m_value;
    std::string m_filter;
    bool m_alt;
};

//-------------------------------------------------------

class PathOutput : public ProcessorOutput {
  public:
    PathOutput() {
      setType(IOType::STRING);
      setColor(color_string);
    }

    // -----------------------------------------------------

    std::shared_ptr<ProcessorOutput> clone() {
      std::shared_ptr<ProcessorOutput> output = std::shared_ptr<ProcessorOutput>(new PathOutput());
      output->setName(name());
      output->setColor(color());
      return output;
    }
};

// -----------------------------------------------------

// IO_SCALAR
class ScalarInput : public ProcessorInput {
  public:
    ScalarInput() {
      setType (IOType::SCALAR);
      setColor(color_number);
    }

    // -----------------------------------------------------

    template <typename ...>
    ScalarInput(float _value, float _min = min(), float _max = max(), bool _alt = false, float _step = step(), ...) : ScalarInput() {
      m_value = _value;
      m_min   = _min;
      m_max   = _max;
      m_alt   = _alt;
      m_step  = _step;

      m_value = std::min(m_max, std::max(m_min, m_value));

      if (m_step == 0.f) {
        if (m_min != min() && m_max != max()) {
          m_step = (m_max - m_min) / 100.0f;
        }
        else {
          m_step = step();
        }
      }
    }

    // -----------------------------------------------------

    template <typename ...>
    ScalarInput(std::vector<std::string>& _params) : ScalarInput() {
      size_t s = _params.size();

      m_value = s >= 1 ? std::stof(_params[0]) : 0.0f;
      m_min   = s >= 2 ? std::stof(_params[1]) : min();
      m_max   = s >= 3 ? std::stof(_params[2]) : max();
      m_alt   = s >= 4 ? _params[3] == "true"  : false;
      m_step  = s >= 5 ? std::stof(_params[4]) : step();

      m_value = std::min(m_max, std::max(m_min, m_value));
    }

    // -----------------------------------------------------

    std::shared_ptr<ProcessorInput> clone() {
      std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new ScalarInput(m_value, m_min, m_max, m_alt, m_step));
      input->setName (name());
      input->setColor(color());
      input->m_isDataOnly = m_isDataOnly;
      return input;
    }

    // -----------------------------------------------------

    // For compatibility (shouldn't be called)
    template <typename ...>
    ScalarInput(...) : ScalarInput()
    {
      sl_assert(false);
    }

    // -----------------------------------------------------

    bool drawTweak();

    // -----------------------------------------------------

    void save(std::ofstream& _stream) {
      _stream << "i_" << getUniqueID() << " = Input({" <<
                 "name = '" << name() << "'" <<
                 ", type = '" << IOType::ToString(type()) << "'" <<
                 ", value = " << m_value <<
                 (m_min != min() ? ", min = " + std::to_string(m_min) : "") <<
                 (m_max != max() ? ", max = " + std::to_string(m_max) : "") <<
                 (m_alt ? ", alt = true" : "") <<
                 (m_step != step() ? ", step = " + std::to_string(m_step) : "") <<
                 "})" << std::endl;
    }

    // -----------------------------------------------------

    std::string getLuaValue() {
      return std::to_string(m_value);
    }

    // -----------------------------------------------------

    static inline float min() { return -std::numeric_limits<float>().max(); }
    static inline float max() { return  std::numeric_limits<float>().max(); }
    static inline float step() { return 1.0f; }

    // -----------------------------------------------------

    float m_value;
    float m_min;
    float m_max;
    float m_step;

    bool  m_alt;
};

// -----------------------------------------------------

class ScalarOutput : public ProcessorOutput
{
  public:
    ScalarOutput() {
      setType (IOType::SCALAR);
      setColor(color_number);
    }

    // -----------------------------------------------------

    std::shared_ptr<ProcessorOutput> clone() {
      std::shared_ptr<ProcessorOutput> output = std::shared_ptr<ProcessorOutput>(new ScalarOutput());
      output->setName (name());
      output->setColor(color());
      return output;
    }
};

// -----------------------------------------------------
// IO_STRING

class StringInput : public ProcessorInput
{
  public:
    StringInput() {
      setType (IOType::STRING);
      setColor(color_string);
    }

    // -----------------------------------------------------

    // For compatibility (shouldn't be called)
    template <typename ...>
    StringInput(...) : StringInput() {
      sl_assert(false);
    }

    // -----------------------------------------------------

    template <typename ...>
    StringInput(std::string _value = "", bool _alt = false, ...) : StringInput() {
      m_value = _value;
      m_alt = _alt;
    }

    // -----------------------------------------------------

    template <typename ...>
    StringInput(std::vector<std::string>& _params) : StringInput()
    {
      size_t s = _params.size();

      m_value = s >= 1 ? _params[0]           : "";
      m_alt   = s >= 2 ? _params[1] == "true" : false;

      std::regex quote("(^[\'\"]|[\'\"]$)");
      std::string unquoted;

      regex_replace(std::back_inserter(unquoted), m_value.begin(), m_value.end(), quote, "$2");

      m_value = unquoted;
    }

    // -----------------------------------------------------

    std::shared_ptr<ProcessorInput> clone() {
      std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new StringInput(m_value));
      input->setName (name());
      input->setColor(color());
      input->m_isDataOnly = m_isDataOnly;
      return input;
    }

    // -----------------------------------------------------

    bool drawTweak();

    // -----------------------------------------------------

    void save(std::ofstream& _stream) {
      _stream << "i_" << getUniqueID() << " = Input({" <<
                 "name = '" << name() << "', " <<
                 "type = '" << IOType::ToString(type()) << "', " <<
                 "value = '" << m_value << "'" <<
                 (m_alt ? ", alt = true" : "") <<
                 "})" << std::endl;
    }

    // -----------------------------------------------------

    std::string getLuaValue() {
      std::regex escaped_chars("([\\[\\]\\{\\}\\\"\\\'])");
      std::string escaped;
      regex_replace(std::back_inserter(escaped), m_value.begin(), m_value.end(), escaped_chars, "\\$1");
      return "'" + escaped + "'";
    }

    // -----------------------------------------------------

    std::string m_value;
    bool m_alt;
};

// -----------------------------------------------------

class StringOutput : public ProcessorOutput
{
  public:
    StringOutput() {
      setType (IOType::STRING);
      setColor(color_string);
    }

    // -----------------------------------------------------

    std::shared_ptr<ProcessorOutput> clone() {
      std::shared_ptr<ProcessorOutput> output = std::shared_ptr<ProcessorOutput>(new StringOutput());
      output->setName (name());
      output->setColor(color());
      return output;
    }
};

// -----------------------------------------------------
// IO_SHAPE

class ShapeInput : public ProcessorInput {
  public:
    ShapeInput() {
      setType(IOType::SHAPE);
      setColor(color_shape);
    }

    // -----------------------------------------------------

    template <typename ...>
    ShapeInput(std::vector<std::string>& ) : ShapeInput() {}

    // -----------------------------------------------------

    template <typename ...>
    ShapeInput(...) : ShapeInput() {
      sl_assert(false);
    }

    // -----------------------------------------------------

    bool drawTweak();

    // -----------------------------------------------------

    std::shared_ptr<ProcessorInput> clone() {
      std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new ShapeInput());
      input->setName (name());
      input->setColor(color());
      input->m_isDataOnly = m_isDataOnly;
      return input;
    }

    // -----------------------------------------------------

    void save(std::ofstream& _stream) {
      _stream << "i_" << getUniqueID() << " = Input({" <<
                 "name = '" << name() << "'" <<
                 ", type = '" << IOType::ToString(type()) << "'" <<
                 "})" << std::endl;
    }

    // -----------------------------------------------------

    std::string getLuaValue() {
      return "Void";
    }
};

// -----------------------------------------------------

class ShapeOutput : public ProcessorOutput {
  public:
    ShapeOutput() {
      setType (IOType::SHAPE);
      setColor(color_shape);
    }

    // -----------------------------------------------------

    std::shared_ptr<ProcessorOutput> clone() {
      std::shared_ptr<ProcessorOutput> output = std::shared_ptr<ProcessorOutput>(new ShapeOutput());
      output->setName (name());
      output->setColor(color());
      return output;
    }
};

// -----------------------------------------------------
// IO_VEC4

class Vec4Input : public ProcessorInput {
  public:
    Vec4Input() {
      setType (IOType::VEC4);
      setColor(color_vec4);
    }

    // -----------------------------------------------------

    // For compatibility (shouldn't be called)
    template <typename ...>
    Vec4Input(...) : Vec4Input() {
      sl_assert(false);
    }

    // -----------------------------------------------------

    template <typename ...>
    Vec4Input(float * _value, float _min = min(), float _max = max(), bool _alt = false, float _step = step(), ...) : Vec4Input() {
      m_min  = _min;
      m_max  = _max;
      m_alt  = _alt;
      m_step = _step;

      for (int i = 0; i < 4; ++i) {
        m_value[i] = std::min(m_max, std::max(m_min, _value[i]));
      }

      if (m_step == 0.f) {
        if (m_min != min() && m_max != max()) {
          m_step = (m_max - m_min) / 100.0f;
        }
        else {
          m_step = step();
        }
      }
    }

    // -----------------------------------------------------

    template <typename ...>
    Vec4Input(std::vector<std::string>& _params) : Vec4Input()
    {
      size_t s = _params.size();

      if (s >= 1) {
        char trash;
        std::istringstream iss(_params[0].c_str());
        for (int i = 0; i < 4; ++i) {
          iss >> trash >> m_value[i];
        }
      } else {
        m_value[0] = 0.0f; m_value[1] = 0.0f; m_value[2] = 0.0f; m_value[3] = 0.0f;
      }
      m_min   = s >= 2 ? std::stof(_params[1]) : min();
      m_max   = s >= 3 ? std::stof(_params[2]) : max();
      m_alt   = s >= 4 ? _params[3].compare(std::string("true")) == 0 : false;
      m_step  = s >= 5 ? std::stof(_params[4]) : step();

      std::cout << _params[3] << " " << _params[3].compare(std::string("true")) << " alt:" << (m_alt?"true":"false") << std::endl;
    }

    // -----------------------------------------------------

    std::shared_ptr<ProcessorInput> clone() {
      std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new Vec4Input(m_value, m_min, m_max, m_alt, m_step));
      input->setName (name());
      input->setColor(color());
      input->m_isDataOnly = m_isDataOnly;
      return input;
    }

    // -----------------------------------------------------

    bool drawTweak();

    // -----------------------------------------------------

    void save(std::ofstream& _stream) {
      _stream << "i_" << getUniqueID() << " = Input({" <<
                 "name = '" << name() << "'" <<
                 ", type = '" << IOType::ToString(type()) << "'" <<
                 ", value = {" << m_value[0] << ", " << m_value[1] << ", " << m_value[2] << ", " << m_value[3] << "}" <<
                 (m_min != min() ? ", min = " + std::to_string(m_min) : "") <<
                 (m_max != max() ? ", max = " + std::to_string(m_max) : "") <<
                 (m_alt ? ", alt = true" : "") <<
                 (m_step != step() ? ", step = " + std::to_string(m_step) : "") <<
                 "})" << std::endl;
    }

    // -----------------------------------------------------

    std::string getLuaValue() {
      std::string s = "{"
          + std::to_string(m_value[0]) + ", "
          + std::to_string(m_value[1]) + ", "
          + std::to_string(m_value[2]) + ", "
          + std::to_string(m_value[3]) + "}";
      return s;
    }

    // -----------------------------------------------------

    static inline float min() { return -std::numeric_limits<float>().max(); }
    static inline float max() { return  std::numeric_limits<float>().max(); }
    static inline float step() { return 1.0f; }

    // -----------------------------------------------------

    float m_value[4];
    float m_min;
    float m_max;
    float m_step;
    bool  m_alt;
};

// -----------------------------------------------------

class Vec4Output : public ProcessorOutput
{
  public:
    Vec4Output() {
      setType (IOType::VEC4);
      setColor(color_vec4);
    }

    // -----------------------------------------------------

    std::shared_ptr<ProcessorOutput> clone() {
      std::shared_ptr<ProcessorOutput> output = std::shared_ptr<ProcessorOutput>(new Vec4Output());
      output->setName (name());
      output->setColor(color());
      return output;
    }
};

// -----------------------------------------------------
// IO_VEC3

class Vec3Input : public ProcessorInput
{
  public:
    Vec3Input() {
      setType (IOType::VEC3);
      setColor(color_vec3);
    }

    // -----------------------------------------------------

    template <typename...>
    Vec3Input(float * _value, float _min = min(), float _max = max(), float _step = step(), ...) : Vec3Input() {
      m_min = _min;
      m_max = _max;
      m_step = _step;

      for (int i = 0; i < 3; ++i) {
        m_value[i] = std::min(m_max, std::max(m_min, _value[i]));
      }

      if (m_step == 0.f) {
        if (m_min != min() && m_max != max()) {
          m_step = (m_max - m_min) / 100.0f;
        }
        else {
          m_step = step();
        }
      }
    }

    // -----------------------------------------------------

    template <typename ...>
    Vec3Input(std::vector<std::string>& _params) : Vec3Input()
    {
      size_t s = _params.size();

      if (s >= 1) {
        char trash;
        std::istringstream iss(_params[0].c_str());
        iss >> trash >> m_value[0] >> trash >> m_value[1] >> trash >> m_value[2];
      } else {
        m_value[0] = 0.0f; m_value[1] = 0.0f; m_value[2] = 0.0f;
      }
      m_min  = s >= 2 ? std::stof(_params[1]) : min();
      m_max  = s >= 3 ? std::stof(_params[2]) : max();
      m_step = s >= 4 ? std::stof(_params[3]) : step();
    }

    // -----------------------------------------------------

    // For compatibility (shouldn't be called)
    template <typename ...>
    Vec3Input(...) : Vec3Input() {
      sl_assert(false);
    }

    // -----------------------------------------------------

    std::shared_ptr<ProcessorInput> clone() {
      std::shared_ptr<ProcessorInput> input = std::shared_ptr<ProcessorInput>(new Vec3Input(m_value, m_min, m_max, m_step));
      input->setName (name());
      input->setColor(color());
      input->m_isDataOnly = m_isDataOnly;
      return input;
    }

    // -----------------------------------------------------

    bool drawTweak();

    // -----------------------------------------------------

    void save(std::ofstream& _stream) {
      _stream << "i_" << getUniqueID() << " = Input({" <<
                 "name = '" << name() << "'" <<
                 ", type = '" << IOType::ToString(type()) << "'" <<
                 ", value = {" << m_value[0] << "," << m_value[1] << "," << m_value[2] << "}" <<
                 (m_min != min() ? ", min = " + std::to_string(m_min) : "") <<
                 (m_max != max() ? ", max = " + std::to_string(m_max) : "") <<
                 (m_step != step() ? ", step = " + std::to_string(m_step) : "") <<
                 "})" << std::endl;
    }

    // -----------------------------------------------------

    std::string getLuaValue() {
      std::string s = "v("
          + std::to_string(m_value[0]) + ", "
          + std::to_string(m_value[1]) + ", "
          + std::to_string(m_value[2]) + ")";
      return s;
    }

    // -----------------------------------------------------

    static inline float min() { return -std::numeric_limits<float>().max(); }
    static inline float max() { return  std::numeric_limits<float>().max(); }
    static inline float step() { return 1.0f; }

    // -----------------------------------------------------

    float m_value[3];
    float m_min;
    float m_max;
    float m_step;
};

class Vec3Output : public ProcessorOutput
{
  public:
    Vec3Output() {
      setType (IOType::VEC3);
      setColor(color_vec3);
    }

    // -----------------------------------------------------

    std::shared_ptr<ProcessorOutput> clone() {
      std::shared_ptr<ProcessorOutput> output = std::shared_ptr<ProcessorOutput>(new Vec3Output());
      output->setName (name());
      output->setColor(color());
      return output;
    }
};

// -----------------------------------------------------

template <typename ... Args>
std::shared_ptr<ProcessorInput> ProcessorInput::create(const std::string& _name, IOType::IOType _type, Args&& ... _args) {
  std::shared_ptr<ProcessorInput> input;
  switch (_type) {
  case IOType::BOOLEAN:
    input = std::shared_ptr<ProcessorInput>(new BoolInput(_args...));
    break;
  case IOType::INTEGER:
    input = std::shared_ptr<ProcessorInput>(new IntInput(_args...));
    break;
  case IOType::IMPLICIT:
    input = std::shared_ptr<ProcessorInput>(new ImplicitInput(_args...));
    break;
  case IOType::LIST:
    input = std::shared_ptr<ProcessorInput>(new ListInput(_args...));
    break;
  case IOType::PATH:
    input = std::shared_ptr<ProcessorInput>(new PathInput(_args...));
    break;
  case IOType::SCALAR:
    input = std::shared_ptr<ProcessorInput>(new ScalarInput(_args...));
    break;
  case IOType::SHAPE:
    input = std::shared_ptr<ProcessorInput>(new ShapeInput(_args...));
    break;
  case IOType::STRING:
    input = std::shared_ptr<ProcessorInput>(new StringInput(_args...));
    break;
  case IOType::VEC3:
    input = std::shared_ptr<ProcessorInput>(new Vec3Input(_args...));
    break;
  case IOType::VEC4:
    input = std::shared_ptr<ProcessorInput>(new Vec4Input(_args...));
    break;
  case IOType::UNDEF:
  default:
    input = std::shared_ptr<ProcessorInput>(new UndefInput());
    break;
  }
  input->setName(_name);
  input->setType(_type);
  return input;
}

// -----------------------------------------------------

} // namespace chill
