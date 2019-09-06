#include "LuaProcessor.h"

#include <LibSL/LibSL.h>
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

namespace Chill
{
  LuaProcessor::LuaProcessor(LuaProcessor &_processor) {
    m_nodepath = _processor.m_nodepath;
    setName(_processor.name());
    setOwner(_processor.owner());
    setColor(_processor.color());

    for (auto input : _processor.inputs()) {
      addInput(input->clone());
    }

    for (auto output : _processor.outputs()) {
      addOutput(output->clone());
    }
  }

  LuaProcessor::LuaProcessor(const std::string &_path) {
    std::regex e("\\\\");
    m_nodepath = regex_replace(_path, e, "/$2");
    m_program  = loadFileIntoString((NodeEditor::NodesFolder() + m_nodepath).c_str());

    setName(removeExtensionFromFileName(extractFileName(m_nodepath)));
    Parse();
  }

  void LuaProcessor::save(std::ofstream& _stream) {
    ImVec4 rgba = ImGui::ColorConvertU32ToFloat4(color());
    _stream << "p_" << getUniqueID() << " = Node({" <<
      "name = '" << name() << "'" <<
      ", x = " << getPosition().x <<
      ", y = " << getPosition().y <<
      ", color = {" << int(rgba.x * 255) << ", " << int(rgba.y * 255) << ", " << int(rgba.z * 255) << "}" <<
      ", path = '" << m_nodepath << "'" <<
      "})" << std::endl;

    /* Saving I/Os is not usefull in general case*/
    // Save inputs
    for (AutoPtr<ProcessorInput> input : inputs()) {
      input->save(_stream);
      _stream << "p_" << getUniqueID() << ":add(i_" << input->getUniqueID() << ")" << std::endl;
    }
    // Save outputs
    for (AutoPtr<ProcessorOutput> output : outputs()) {
      output->save(_stream);
      _stream << "p_" << getUniqueID() << ":add(o_" << output->getUniqueID() << ")" << std::endl;
    }
  }

  void LuaProcessor::iceSL(std::ofstream& _stream) {
    //write the current Id of the node

    std::string code = "--[[ " + name() + " ]]--\n";
    code += "setfenv(1, _G0)  --go back to global initialization\n";
    code += "__currentNodeId = " + std::to_string((int64_t)this) + "\n";

    if (isDirty() || isEmiter()) {
      code += "setDirty(__currentNodeId)\n";
    }

    for (auto input : inputs()) {
      // tweak
      if (input->m_link.isNull()) {
        code += "__input[\"" + std::string(input->name()) + "\"] = {" + input->getLuaValue() + ", 0}\n";
      }
      // input
      else {
        std::string s2 = std::to_string((int64_t)input->m_link->owner());
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
      if (!input->m_link.isNull()) {
        std::string s2 = std::to_string((int64_t)input->m_link->owner());
        code += ", " + s2;
      }
    }

    code += "})) then\n\
setDirty(__currentNodeId)\n";


    code += loadFileIntoString((NodeEditor::NodesFolder() + m_nodepath).c_str());

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
      std::regex input_regex("input" + REGEX_WSPACES + "\\(" + REGEX_WSPACES + REGEX_STRING + REGEX_WSPACES
        + "," + REGEX_WSPACES + REGEX_STRING + REGEX_WSPACES
        + "(?:," + REGEX_WSPACES + "((?:,?" + REGEX_WSPACES + REGEX_PARAM + REGEX_WSPACES + ")*?)|)\\)");
      std::smatch sm;
      while (regex_search(outcome, sm, input_regex)) {
        std::smatch parameters;
        std::regex param_regex("^,?" + REGEX_WSPACES + "?(" + REGEX_PARAM  + ")" + REGEX_WSPACES + "?");
        std::string outcome2 = sm[3].str();

        std::vector<std::string> params;
        while (!outcome2.empty() && regex_search(outcome2, parameters, param_regex)) {
          params.push_back(parameters[1]);
          outcome2 = parameters.suffix().str();
        }
        addInput(sm[1], IOType::FromString(sm[2]), params);
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

  AutoPtr<ProcessorInput> LuaProcessor::addInput(AutoPtr<ProcessorInput> _input) {
    _input->setOwner(this);
    if (input(_input->name()).isNull()) {
      Processor::addInput(_input);
    }
    else {
      replaceInput(_input->name(), _input);
    }
    return _input;
  }

  AutoPtr<ProcessorOutput> LuaProcessor::addOutput(AutoPtr<ProcessorOutput> _output) {
    _output->setOwner(this);
    if (output(_output->name()).isNull()) {
      Processor::addOutput(_output);
    }
    else {
      replaceOutput(_output->name(), _output);
    }
    return _output;
  }
}
