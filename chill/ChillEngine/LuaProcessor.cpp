#include "LuaProcessor.h"

#include <regex>

#include "NodeEditor.h"

const std::string REGEX_WSPACES = "\\s*";
const std::string REGEX_COMMENT = "(--\\[\\[[\\s\\S]*?\\]\\]--|--[^\\n]*)";
const std::string REGEX_STRING  = "[\\\"\\\']([\\S\\s]*?)[\\\"\\\']";
const std::string REGEX_INT     = "(-?\\d+)";
const std::string REGEX_SCALAR  = "(-?(?:\\d*[.]\\d+|\\d+[.]\\d*)(?:[Ee][+-]?\\d+)?)";
const std::string REGEX_NUMBER  = "(?:" + REGEX_SCALAR + "|" + REGEX_INT + ")";
const std::string REGEX_BOOL    = "(true|false)";
const std::string REGEX_NAMED   = "^(?:\\s)*([a-zA-Z_][\\w_]*)\\s*=\\s*";
const std::string REGEX_TABLE   = "\\{(?:(,?\\s*(?:" + REGEX_NAMED + "|)(?:" + REGEX_STRING + "|" + REGEX_NUMBER + "|" + REGEX_BOOL + ")\\s*?)*)\\}";
const std::string REGEX_PARAM   = "(?:" + REGEX_STRING + "|" + REGEX_NUMBER + "|" + REGEX_BOOL + "|" + REGEX_TABLE + ")";

namespace chill {
  LuaProcessor::LuaProcessor(LuaProcessor &_processor) {
    m_nodepath = _processor.m_nodepath;
    setName(_processor.name());
    setOwner(_processor.owner());
    setColor(_processor.color());
    m_program = loadFileIntoString(getUserDir() / "chill-nodes" / m_nodepath);

    for (auto input : _processor.inputs()) {
      addInput(input->clone());
    }

    for (auto output : _processor.outputs()) {
      addOutput(output->clone());
    }
  }

  LuaProcessor::LuaProcessor(const fs::path& _path) {
    m_nodepath = fs::path(_path.generic_string());
    m_program = loadFileIntoString(getUserDir() / "chill-nodes" / _path);

    setName(m_nodepath.filename().replace_extension().generic_string());
    Parse();
  }

  void LuaProcessor::save(std::ofstream& _stream) {
    std::regex e("\\\\");
    std::string path = regex_replace(m_nodepath.generic_string(), e, "/$2");

    ImVec4 rgba = ImGui::ColorConvertU32ToFloat4(color());
    _stream << "p_" << getUniqueID() << " = Node({" <<
      "name = '" << name() << "'" <<
      ", x = " << getPosition().x <<
      ", y = " << getPosition().y <<
      ", color = {" << int(rgba.x * 255) << ", " << int(rgba.y * 255) << ", " << int(rgba.z * 255) << "}" <<
      ", path = '" << path << "'" <<
      "})" << std::endl;

    /* Saving I/Os is not usefull in general case*/
    // Save inputs
    for (ProcessorInputPtr input : inputs()) {
      input->save(_stream);
      _stream << "p_" << getUniqueID() << ":add(i_" << input->getUniqueID() << ")" << std::endl;
    }
    // Save outputs
    for (ProcessorOutputPtr output : outputs()) {
      output->save(_stream);
      _stream << "p_" << getUniqueID() << ":add(o_" << output->getUniqueID() << ")" << std::endl;
    }
  }

  void LuaProcessor::iceSL(std::ofstream& _stream) {
    //write the current Id of the node

    std::string code = "--[[ " + name() + " ]]--\n";
    code += "setfenv(1, _G0)  --go back to global initialization\n";
    code += "__currentNodeId = " + std::to_string(getUniqueID()) + "\n";

    if (isDirty() || isEmiter()) {
      code += "setDirty(__currentNodeId)\n";
    }

    for (auto input : inputs()) {
      // tweak
      if (!input->m_link) {
        code += "__input[\"" + std::string(input->name()) + "\"] = {" + input->getLuaValue() + ", 0}\n";
      }
      // input
      else {
        std::string s2 = std::to_string(input->m_link->owner()->getUniqueID());
        code += "__input[\"" + std::string(input->name()) + "\"] = {" + input->m_link->name() + s2 + "," + s2 + "}\n";
      }
    }

    //TODO: CLEAN THIS !!!!
    code += "\
_Gcurrent = {} -- clear _Gcurrent\n\
setmetatable(_Gcurrent, { __index = _G0 }) --copy index from _G0\n\
setfenv(1, _Gcurrent)    --set it\n\
";

    code += "if (isDirty({__currentNodeId";

    for (auto input : inputs()) {
      if (input->m_link) {
        std::string s2 = std::to_string(input->m_link->owner()->getUniqueID());
        code += ", " + s2;
      }
    }

    code += "})) then\n\
setDirty(__currentNodeId)\n";


    code += loadFileIntoString(getUserDir() / "chill-nodes" / m_nodepath);

    if (getState() == EMITING) {
      for (auto output : outputs()) {
        if (output->isEmitable()) {
          code += "emit( _G['"+ std::string(output->name()) +"'..__currentNodeId])" + "\n";
        }
      }
    }
    if (getState() == DISABLED) {
      for (auto output : outputs()) {
        if (output->isEmitable()) {
          code += "_G['" + std::string(output->name()) + "'..__currentNodeId] = Void" + "\n";
        }
      }
    }

    code += "\nend --vb";

    _stream << code << std::endl;
  }



  void LuaProcessor::ParseInput() {
    std::string toParse = m_program;
    std::string uncommented;
    std::regex nocomment(REGEX_COMMENT);
    regex_replace(std::back_inserter(uncommented), toParse.begin(), toParse.end(), nocomment, "$2");

    try {
      std::string outcome = uncommented;
      std::regex input_regex("(input|data)" + REGEX_WSPACES + "\\(" + REGEX_WSPACES + REGEX_STRING + REGEX_WSPACES
        + "," + REGEX_WSPACES + REGEX_STRING + REGEX_WSPACES
        + "(?:," + REGEX_WSPACES + "((?:,?" + REGEX_WSPACES + REGEX_PARAM + REGEX_WSPACES + ")*?)|)\\)");
      std::smatch sm;
      while (regex_search(outcome, sm, input_regex)) {
        std::smatch parameters;
        std::regex param_regex("^,?" + REGEX_WSPACES + "?(" + REGEX_PARAM  + ")" + REGEX_WSPACES + "?");
        std::string outcome2 = sm[4].str();

        std::vector<std::string> params;
        while (!outcome2.empty() && regex_search(outcome2, parameters, param_regex)) {
          params.push_back(parameters[1]);
          outcome2 = parameters.suffix().str();
        }
        auto input = addInput(sm[2], IOType::FromString(sm[3]), params);
        
        if (sm[1] == "data")
          input->m_isDataOnly = true;
        outcome = sm.suffix().str();
      }
    }
    catch (const std::regex_error& e) {
      std::cout << "ParseInput: regex_error caught: " << e.what() << '\n';
    }
  }

  void LuaProcessor::ParseOutput() {
    std::string toParse = m_program;
    std::string uncommented;
    std::regex nocomment(REGEX_COMMENT);
    regex_replace(std::back_inserter(uncommented), toParse.begin(), toParse.end(), nocomment, "$2");

    try {
      std::string outcome = uncommented;
      std::regex outputEx("output" + REGEX_WSPACES + "\\(" + REGEX_WSPACES + REGEX_STRING + REGEX_WSPACES
        + "," + REGEX_WSPACES + REGEX_STRING + REGEX_WSPACES
        + "(," + REGEX_WSPACES + ".*" + REGEX_WSPACES + ")*?\\)");
      std::smatch sm;
      while (regex_search(outcome, sm, outputEx)) {
        addOutput(sm[1], IOType::FromString(sm[2]), IOType::FromString(sm[2]) == IOType::SHAPE);
        outcome = sm.suffix().str();
      }
    }
    catch (const std::regex_error& e) {
      std::cout << "ParseOutput: regex_error caught: " << e.what() << '\n';
    }
  }

  void LuaProcessor::ParseOptional() {
    std::string toParse = m_program;
    std::string uncommented;
    std::regex nocomment(REGEX_COMMENT);
    regex_replace(std::back_inserter(uncommented), toParse.begin(), toParse.end(), nocomment, "$2");

    try {
      std::string outcome = uncommented;
      std::regex outputEx("emit" + REGEX_WSPACES + "\\(.*\\)");
      std::regex color("setColor" + REGEX_WSPACES + "\\(\\s*" + REGEX_NUMBER + "\\s*,\\s*" + REGEX_NUMBER + "\\s*,\\s*" + REGEX_NUMBER + "\\s*\\)");
      std::smatch sm;
      if (regex_search(outcome, sm, outputEx)) {
        setEmiter();
      }
      if (regex_search(outcome, sm, color)) {
        setColor(ImColor(
          atoi(sm[2].str().c_str()),
          atoi(sm[4].str().c_str()),
          atoi(sm[6].str().c_str())));
      }
    }
    catch (const std::regex_error& e) {
      std::cout << "regex_error caught: " << e.what() << '\n';
    }
  }

  ProcessorInputPtr LuaProcessor::addInput(ProcessorInputPtr _input) {
    _input->setOwner(this);
    if (!input(_input->name())) {
      Processor::addInput(_input);
    }
    else {
      replaceInput(_input->name(), _input);
    }
    return _input;
  }

  ProcessorOutputPtr LuaProcessor::addOutput(ProcessorOutputPtr _output) {
    _output->setOwner(this);
    if (!output(_output->name())) {
      Processor::addOutput(_output);
    }
    else {
      replaceOutput(_output->name(), _output);
    }
    return _output;
  }
}
