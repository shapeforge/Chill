#pragma once
namespace Chill
{
  class ProcessingGraph;
}

namespace Chill
{
  class VisualComment;
}

#include <LibSL.h>
#include "UI.h"



namespace Chill
{

  class VisualComment : public SelectableUI {
  public:


    VisualComment(VisualComment &_copy);

    VisualComment() { m_size = ImVec2(200, 200); 
    m_comment = "Comment";
    m_selected = false;
    m_edit = false;
    m_title_size = ImVec2(200, 50);
    };
    void setOwner(Chill::ProcessingGraph* proc) {
      this->m_owner = proc;
    }

    Chill::ProcessingGraph* owner() {
      return this->m_owner;
    }



    bool draw();
    virtual AutoPtr<SelectableUI>clone() {
      return AutoPtr<SelectableUI>(new VisualComment(*this));
    };

    ImVec2 m_title_size;

  private:
    Chill::ProcessingGraph* m_owner;
    std::string m_comment = "";
    char m_charComment[255] = "\0";
  };

}