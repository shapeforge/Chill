#include "GraphSaver.h"
#include "NodeEditor.h"
#include "LuaProcessor.h"

#include <array>


using namespace luabind;

namespace Chill
{
  class Lua_Input {
  protected:
    AutoPtr<ProcessorInput> m_Ptr;
  public:
    //-------------------------------------------------------
    Lua_Input() {
      m_Ptr = ProcessorInput::create("undef", IOType::UNDEF);
    };

    //-------------------------------------------------------
    Lua_Input(const object& table) {
      std::string name = "undef";
      IOType::IOType type = IOType::UNDEF;

      bool bool_value = false;

      int int_value = 0;
      int int_min = std::numeric_limits<int>().min();
      int int_max = std::numeric_limits<int>().max();
      int int_step = 1;

      float float_min = std::numeric_limits<float>().min();
      float float_max = std::numeric_limits<float>().max();
      float float_step = 0.1f;
      
      bool alt = false;

      if (table.is_valid())
      {
        if (!(table["name"] && table["type"])) {
          std::cerr << "ERROR: Lua_Input, missing arguments" << std::endl;
          return;
        }

        name = object_cast<std::string>(table["name"]);
        type = IOType::FromString(object_cast<std::string>(table["type"]));

        if (table["alt"]) alt = object_cast<bool>(table["alt"]);

        switch (type) {
        case IOType::BOOLEAN:
          if (table["value"]) bool_value = object_cast<bool>(table["value"]);
          m_Ptr = ProcessorInput::create(name, type, bool_value);
          break;

        case IOType::INTEGER:
          if (table["value"]) int_value = object_cast<int>(table["value"]);
          m_Ptr = ProcessorInput::create(name, type, int_value, int_min, int_max, alt, int_step);
          break;

        case IOType::SCALAR:
          float float_value;
          if (table["value"]) float_value = object_cast<float>(table["value"]);
          float_min = table["min"] ? object_cast<float>(table["min"]) : ScalarInput::min();
          float_max = table["max"] ? object_cast<float>(table["max"]) : ScalarInput::max();
          float_step = table["step"] ? object_cast<float>(table["step"]) : 0.f;
          m_Ptr = ProcessorInput::create(name, type, float_value, float_min, float_max, alt, float_step);
          break;

        case IOType::VEC3:
          float float_3value[3];
          if (table["value"]) {
            const object & values = table["value"];
            if (values.is_valid()) {
              if (values[1])
                float_3value[0] = object_cast<float>(values[1]);
              if (values[2])
                float_3value[1] = object_cast<float>(values[2]);
              if (values[3])
                float_3value[2] = object_cast<float>(values[3]);
            }
          }

          float_min = table["min"] ? object_cast<float>(table["min"]) : Vec3Input::min();
          float_max = table["max"] ? object_cast<float>(table["max"]) : Vec3Input::max();
          m_Ptr = ProcessorInput::create(name, type, float_3value, float_min, float_max, alt, float_step);
          break;

        case IOType::VEC4:
          float float_4value[4];
          float_min = table["min"] ? object_cast<float>(table["min"]) : Vec4Input::min();
          float_max = table["max"] ? object_cast<float>(table["max"]) : Vec4Input::max();
          break;
        case IOType::UNDEF:
        default:
          m_Ptr = ProcessorInput::create(name, type);
          break;
        }

      }
    }

    //-------------------------------------------------------
    AutoPtr<ProcessorInput>& ptr() {
      return m_Ptr;
    }
  };

  class Lua_Output {
  protected:
    AutoPtr<ProcessorOutput> m_Ptr;
  public:
    //-------------------------------------------------------
    Lua_Output() {
      m_Ptr = ProcessorOutput::create("undef", IOType::UNDEF);
    };

    //-------------------------------------------------------
    Lua_Output(const object& table) {
      std::string name = "undef";
      IOType::IOType type = IOType::UNDEF;

      if (table.is_valid())
      {
        if (table["name"]) {
          name = object_cast<std::string>(table["name"]);
        }
        if (table["type"]) {
          type = IOType::FromString(object_cast<std::string>(table["type"]));
        }
      }

      m_Ptr = ProcessorOutput::create(name, type);
    }

    //-------------------------------------------------------
    AutoPtr<ProcessorOutput>& ptr() {
      return m_Ptr;
    }
  };

  class Lua_Processor {
  protected:
    AutoPtr<Processor> m_Ptr;
  public:
    //-------------------------------------------------------
    Lua_Processor() {
      m_Ptr = AutoPtr<Processor>(new Processor());
    };

    //-------------------------------------------------------
    Lua_Processor(const std::string& name_) {
      m_Ptr = AutoPtr<Processor>(new Processor(name_));
    };

    //-------------------------------------------------------
    Lua_Processor(const object& table) {
      std::string name = "Processor";
      ImColor color = ui_cyan;
      ImVec2 pos(0, 0);

      if (table.is_valid())
      {
        if (table["name"]) name = object_cast<std::string>(table["name"]);
        if (table["x"]) pos.x = object_cast<float>(table["x"]);
        if (table["y"]) pos.y = object_cast<float>(table["y"]);

        if (table["color"]) {
          const object & rgb = table["color"];
          if (rgb.is_valid()) {
            if (rgb[1] && rgb[2] && rgb[3]) {
              color = ImColor(object_cast<int>(rgb[1]), object_cast<int>(rgb[2]), object_cast<int>(rgb[3]));
            }
          }
        }
      }

      m_Ptr = AutoPtr<Processor>(new Processor(name));
      m_Ptr->setPosition(pos);
      m_Ptr->setColor(color);
    }

    //-------------------------------------------------------
    AutoPtr<Processor>& ptr() {
      return m_Ptr;
    }

    //-------------------------------------------------------
    void addInput(Lua_Input& input) {
      m_Ptr->addInput(input.ptr());
    }

    //-------------------------------------------------------
    void addOutput(Lua_Output& output) {
      m_Ptr->addOutput(output.ptr());
    }
  };

  class Lua_Graph : public Lua_Processor {
  public:
    //-------------------------------------------------------
    Lua_Graph() {
      m_Ptr = (AutoPtr<Processor>)AutoPtr<ProcessingGraph>(new ProcessingGraph());
    };

    //-------------------------------------------------------
    Lua_Graph(const std::string& name_) {
      m_Ptr = (AutoPtr<Processor>)AutoPtr<ProcessingGraph>(new ProcessingGraph(name_));
    };

    //-------------------------------------------------------
    Lua_Graph(const object& table) {
      std::string name = "Graph";
      ImColor color = ui_cyan;
      ImVec2 pos(0, 0);

      if (table.is_valid())
      {
        if (table["name"]) name = object_cast<std::string>(table["name"]);
        if (table["x"]) pos.x = object_cast<float>(table["x"]);
        if (table["y"]) pos.y = object_cast<float>(table["y"]);

        if (table["color"]) {
          const object & rgb = table["color"];
          if (rgb.is_valid()) {
            if (rgb[1] && rgb[2] && rgb[3]) {
              color = ImColor(object_cast<int>(rgb[1]), object_cast<int>(rgb[2]), object_cast<int>(rgb[3]));
            }
          }
        }
      }

      m_Ptr = AutoPtr<Processor>(AutoPtr<ProcessingGraph>(new ProcessingGraph(name)));
      m_Ptr->setPosition(pos);
      m_Ptr->setColor(color);
    }

    //-------------------------------------------------------
    void addProcessor(Lua_Processor& proc) {
      AutoPtr<ProcessingGraph>(m_Ptr)->addProcessor(proc.ptr());
    }
  };

  class Lua_Node : public Lua_Processor {
  public:
    //-------------------------------------------------------
    Lua_Node() {
      m_Ptr = AutoPtr<Processor>(AutoPtr<ProcessingGraph>(new ProcessingGraph()));
    };

    //-------------------------------------------------------
    Lua_Node(const std::string& name_) {
      m_Ptr = AutoPtr<Processor>(AutoPtr<ProcessingGraph>(new ProcessingGraph(name_)));
    };

    //-------------------------------------------------------
    Lua_Node(const object& table) {
      std::string name = "Node";
      std::string path = "";
      ImColor     color = ui_cyan;
      ImVec2      pos(0, 0);

      if (table.is_valid())
      {
        if (table["name"]) name = object_cast<std::string>(table["name"]);
        if (table["path"]) path = object_cast<std::string>(table["path"]);
        if (table["x"]) pos.x = object_cast<float>(table["x"]);
        if (table["y"]) pos.y = object_cast<float>(table["y"]);

        if (table["color"]) {
          const object & rgb = table["color"];
          if (rgb.is_valid()) {
            if (rgb[1] && rgb[2] && rgb[3]) {
              color = ImColor(object_cast<int>(rgb[1]), object_cast<int>(rgb[2]), object_cast<int>(rgb[3]));
            }
          }
        }
      }

      AutoPtr<LuaProcessor> lua = AutoPtr<LuaProcessor>(new LuaProcessor(path));
      m_Ptr = (AutoPtr<Processor>)lua;
      m_Ptr->setName(name);
      m_Ptr->setPosition(pos);
      m_Ptr->setColor(color);
    }
  };

  //-------------------------------------------------------
  static void lua_connect(Lua_Output& output, Lua_Input& input) {
    ProcessingGraph::connect(output.ptr(), input.ptr());
  }

  //-------------------------------------------------------
  static void setAsMainGraph(Lua_Graph graph) {
    NodeEditor::Instance()->setMainGraph(AutoPtr<ProcessingGraph>(graph.ptr()).raw());
  }

  //-------------------------------------------------------
  void print(const std::string& string) {
    std::cout << string << std::endl;
  }

  //-------------------------------------------------------
  void GraphSaver::execute(const char* path) {   
    int ret = luaL_dofile(m_LuaState, path);
  }

  //-------------------------------------------------------
  void GraphSaver::registerBindings(lua_State *L_)
  {
    module(L_)
      [
      class_<Lua_Processor>("Processor")
        .def(constructor<>())
        .def(constructor<const std::string&>())
        .def(constructor<const object&>())
        .def("add", &Lua_Processor::addInput)
        .def("add", &Lua_Processor::addOutput),
      class_<Lua_Graph, Lua_Processor>("Graph")
        .def(constructor<>())
        .def(constructor<const std::string&>())
        .def(constructor<const object&>())
        .def("add", &Lua_Graph::addInput)
        .def("add", &Lua_Graph::addOutput)
        .def("add", &Lua_Graph::addProcessor),
      class_<Lua_Node, Lua_Processor>("Node")
        .def(constructor<>())
        .def(constructor<const std::string&>())
        .def(constructor<const object&>()),
      class_<Lua_Input>("Input")
        .def(constructor<>())
        .def(constructor<const object&>()),
      class_<Lua_Output>("Output")
        .def(constructor<>())
        .def(constructor<const object&>()),
      def("connect", &lua_connect),
      def("set_graph", &setAsMainGraph)
      ];

    module(L_)
      [
        def("print", &print)
      ];
  }
  
}