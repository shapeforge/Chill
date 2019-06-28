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

    VisualComment() {
      m_size       = ImVec2(200, 200);
      m_title_size = ImVec2(200, 50);
      m_name       = "Comment";
      m_comment    = "Content";
      m_selected   = false;
      m_edit       = false;
    };

    bool draw();
    virtual AutoPtr<SelectableUI> VisualComment::clone() {
      return AutoPtr<SelectableUI>(new VisualComment(*this));
    };

    ImVec2 m_title_size;

  private:
    std::string m_comment = "";
  };

}