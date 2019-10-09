#include "GraphSaver.h"

#include <array>

#include "LuaProcessor.h"
#include "NodeEditor.h"

namespace chill {

class Lua_Input {
  protected:
    std::shared_ptr<ProcessorInput> m_Ptr;
  public:

    //-------------------------------------------------------

    Lua_Input() {
      m_Ptr = ProcessorInput::create("undef", IOType::UNDEF);
    }

    //-------------------------------------------------------

    Lua_Input(const luabind::object& table) {
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

      if (table.is_valid()) {
        if (!(table["name"] && table["type"])) {
          std::cerr << "ERROR: Lua_Input, missing arguments" << std::endl;
          return;
        }

        name = luabind::object_cast<std::string>(table["name"]);
        type = IOType::FromString(luabind::object_cast<std::string>(table["type"]));

        if (table["alt"]) alt = luabind::object_cast<bool>(table["alt"]);

        switch (type) {
        case IOType::BOOLEAN:
          if (table["value"]) bool_value = luabind::object_cast<bool>(table["value"]);
          m_Ptr = ProcessorInput::create(name, type, bool_value);
          break;

        case IOType::INTEGER:
          if (table["value"]) int_value = luabind::object_cast<int>(table["value"]);
          m_Ptr = ProcessorInput::create(name, type, int_value, int_min, int_max, alt, int_step);
          break;

        case IOType::REAL:
          float float_value;
          if (table["value"]) float_value = luabind::object_cast<float>(table["value"]);
          float_min = table["min"] ? luabind::object_cast<float>(table["min"]) : RealInput::min();
          float_max = table["max"] ? luabind::object_cast<float>(table["max"]) : RealInput::max();
          float_step = table["step"] ? luabind::object_cast<float>(table["step"]) : 0.f;
          m_Ptr = ProcessorInput::create(name, type, float_value, float_min, float_max, alt, float_step);
          break;

        case IOType::VEC3:
          float float_3value[3];
          if (table["value"]) {
            const luabind::object & values = table["value"];
            if (values.is_valid()) {
              if (values[1])
                float_3value[0] = luabind::object_cast<float>(values[1]);
              if (values[2])
                float_3value[1] = luabind::object_cast<float>(values[2]);
              if (values[3])
                float_3value[2] = luabind::object_cast<float>(values[3]);
            }
          }

          float_min = table["min"] ? luabind::object_cast<float>(table["min"]) : Vec3Input::min();
          float_max = table["max"] ? luabind::object_cast<float>(table["max"]) : Vec3Input::max();
          m_Ptr = ProcessorInput::create(name, type, float_3value, float_min, float_max, alt, float_step);
          break;

        case IOType::VEC4:
          float_min = table["min"] ? luabind::object_cast<float>(table["min"]) : Vec4Input::min();
          float_max = table["max"] ? luabind::object_cast<float>(table["max"]) : Vec4Input::max();
          break;
        case IOType::UNDEF:
        default:
          m_Ptr = ProcessorInput::create(name, type);
          break;
        }
      }
    }

    //-------------------------------------------------------

    std::shared_ptr<ProcessorInput>& ptr() {
      return m_Ptr;
    }

}; // class Lua_Input

class Lua_Output {
  protected:
    std::shared_ptr<ProcessorOutput> m_Ptr;

  public:

    //-------------------------------------------------------

    Lua_Output() {
      m_Ptr = ProcessorOutput::create("undef", IOType::UNDEF, false);
    }

    //-------------------------------------------------------

    Lua_Output(const luabind::object& table) {
      std::string name = "undef";
      IOType::IOType type = IOType::UNDEF;

      if (table.is_valid()) {
        if (table["name"])
          name = luabind::object_cast<std::string>(table["name"]);
        if (table["type"])
          type = IOType::FromString(luabind::object_cast<std::string>(table["type"]));
      }

      m_Ptr = ProcessorOutput::create(name, type, type == IOType::SHAPE);
    }

    //-------------------------------------------------------

    std::shared_ptr<ProcessorOutput>& ptr() {
      return m_Ptr;
    }

}; // class Lua_Ouput

//-------------------------------------------------------

class Lua_Processor {
  public:
    //-------------------------------------------------------

    Lua_Processor() {
      m_Ptr = std::shared_ptr<Processor>(new Processor());
    }

    //-------------------------------------------------------

    Lua_Processor(const std::string& name_) {
      m_Ptr = std::shared_ptr<Processor>(new Processor(name_));
    }

    //-------------------------------------------------------

    Lua_Processor(const luabind::object& table) {
      std::string name = "Processor";
      ImColor color = ui_cyan;
      ImVec2 pos(0, 0);

      if (table.is_valid())
      {
        if (table["name"]) name = luabind::object_cast<std::string>(table["name"]);
        if (table["x"]) pos.x = luabind::object_cast<float>(table["x"]);
        if (table["y"]) pos.y = luabind::object_cast<float>(table["y"]);

        if (table["color"]) {
          const  luabind::object& rgb = table["color"];
          if (rgb.is_valid()) {
            if (rgb[1] && rgb[2] && rgb[3]) {
              color = ImColor(luabind::object_cast<int>(rgb[1]), luabind::object_cast<int>(rgb[2]), luabind::object_cast<int>(rgb[3]));
            }
          }
        }
      }

      m_Ptr = std::shared_ptr<Processor>(new Processor(name));
      m_Ptr->setPosition(pos);
      m_Ptr->setColor(color);
    }

    //-------------------------------------------------------

    std::shared_ptr<Processor>& ptr() {
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

  protected:
    std::shared_ptr<Processor> m_Ptr;
}; // class Lua_Processor

class Lua_Graph : public Lua_Processor {
  public:

    //-------------------------------------------------------

    Lua_Graph() {
      m_Ptr = std::shared_ptr<Processor>(new ProcessingGraph());
    }

    //-------------------------------------------------------

    Lua_Graph(const std::string& _name) {
      m_Ptr = std::shared_ptr<Processor>(new ProcessingGraph(_name));
    }

    //-------------------------------------------------------

    Lua_Graph(const luabind::object& table) {
      std::string name = "Graph";
      ImColor color = ui_cyan;
      ImVec2 pos(0, 0);

      if (table.is_valid()) {
        if (table["name"]) name = luabind::object_cast<std::string>(table["name"]);
        if (table["x"]) pos.x = luabind::object_cast<float>(table["x"]);
        if (table["y"]) pos.y = luabind::object_cast<float>(table["y"]);

        if (table["color"]) {
          const luabind::object & rgb = table["color"];
          if (rgb.is_valid()) {
            if (rgb[1] && rgb[2] && rgb[3]) {
              color = ImColor(luabind::object_cast<int>(rgb[1]), luabind::object_cast<int>(rgb[2]), luabind::object_cast<int>(rgb[3]));
            }
          }
        }
      }

      m_Ptr = std::shared_ptr<Processor>(std::shared_ptr<ProcessingGraph>(new ProcessingGraph(name)));
      m_Ptr->setPosition(pos);
      m_Ptr->setColor(color);
    }

    //-------------------------------------------------------

    void addProcessor(Lua_Processor& proc){
      std::static_pointer_cast<ProcessingGraph>(m_Ptr)->addProcessor(proc.ptr());
    }

}; // class Lua_Graph

//-------------------------------------------------------

class Lua_Node : public Lua_Processor {
  public:

    //-------------------------------------------------------

    Lua_Node() {
      m_Ptr = std::shared_ptr<Processor>(std::shared_ptr<ProcessingGraph>(new ProcessingGraph()));
    }

    //-------------------------------------------------------

    Lua_Node(const std::string& name_) {
      m_Ptr = std::shared_ptr<Processor>(std::shared_ptr<ProcessingGraph>(new ProcessingGraph(name_)));
    }

    //-------------------------------------------------------

    Lua_Node(const luabind::object& table) {
      std::string name = "Node";
      fs::path path;
      ImColor     color = ui_cyan;
      ImVec2      pos(0, 0);

      if (table.is_valid()) {
        if (table["name"]) name = luabind::object_cast<std::string>(table["name"]);
        if (table["path"]) path = luabind::object_cast<std::string>(table["path"]);
        if (table["x"]) pos.x = luabind::object_cast<float>(table["x"]);
        if (table["y"]) pos.y = luabind::object_cast<float>(table["y"]);

        if (table["color"]) {
          const luabind::object& rgb = table["color"];
          if (rgb.is_valid()) {
            if (rgb[1] && rgb[2] && rgb[3]) {
              color = ImColor(luabind::object_cast<int>(rgb[1]), luabind::object_cast<int>(rgb[2]), luabind::object_cast<int>(rgb[3]));
            }
          }
        }
      }

      std::shared_ptr<LuaProcessor> lua = std::shared_ptr<LuaProcessor>(new LuaProcessor(path));
      m_Ptr = static_cast<std::shared_ptr<Processor>>(lua);
      m_Ptr->setName(name);
      m_Ptr->setPosition(pos);
      m_Ptr->setColor(color);
    }
};

//-------------------------------------------------------

static void lua_connect(
    Lua_Output& output,
    Lua_Input& input) {
  ProcessingGraph::connect(output.ptr(), input.ptr());
}

//-------------------------------------------------------

static void setAsMainGraph(Lua_Graph graph) {
  NodeEditor::Instance()->setMainGraph(std::static_pointer_cast<ProcessingGraph>(graph.ptr()));
}

//-------------------------------------------------------

void print(const std::string& string) {
  std::cout << string << std::endl;
}

//-------------------------------------------------------

void GraphSaver::execute(const fs::path* path) {
  luaL_dofile(m_LuaState, path->generic_string().c_str());
}

//-------------------------------------------------------

void GraphSaver::registerBindings(lua_State *L_) {
  luabind::module(L_)
      [
      luabind::class_<Lua_Processor>("Processor")
      .def(luabind::constructor<>())
      .def(luabind::constructor<const std::string&>())
      .def(luabind::constructor<const luabind::object&>())
      .def("add", &Lua_Processor::addInput)
      .def("add", &Lua_Processor::addOutput),
      
      luabind::class_<Lua_Graph, Lua_Processor>("Graph")
      .def(luabind::constructor<>())
      .def(luabind::constructor<const std::string&>())
      .def(luabind::constructor<const luabind::object&>())
      .def("add", &Lua_Graph::addInput)
      .def("add", &Lua_Graph::addOutput)
      .def("add", &Lua_Graph::addProcessor),

      luabind::class_<Lua_Node, Lua_Processor>("Node")
      .def(luabind::constructor<>())
      .def(luabind::constructor<const std::string&>())
      .def(luabind::constructor<const luabind::object&>()),

      luabind::class_<Lua_Input>("Input")
      .def(luabind::constructor<>())
      .def(luabind::constructor<const luabind::object&>()),

      luabind::class_<Lua_Output>("Output")
      .def(luabind::constructor<>())
      .def(luabind::constructor<const luabind::object&>()),

      luabind::def("connect", &lua_connect),
      luabind::def("set_graph", &setAsMainGraph)
      ];

  luabind::module(L_)
      [
      luabind::def("print", &print)
      ];
}

} // namespace chill
