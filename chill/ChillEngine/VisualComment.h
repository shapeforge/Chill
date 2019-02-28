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

    bool draw();
    virtual AutoPtr<SelectableUI> VisualComment::clone() {
      return AutoPtr<SelectableUI>(new VisualComment(*this));
    };

    ImVec2 m_title_size;

  private:
    std::string m_comment = "";
    char m_charComment[255] = "\0";
  };

}