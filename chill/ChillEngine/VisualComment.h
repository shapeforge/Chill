#pragma once

#include <memory>

#include "UI.h"

namespace chill {
class ProcessingGraph;
class VisualComment;
}


namespace chill {
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
    }

    bool draw() override;

    std::shared_ptr<SelectableUI> clone() override {
      return std::shared_ptr<SelectableUI>(new VisualComment(*this));
    }

    ImVec2 m_title_size;

  private:
    std::string m_comment = "";
};
}
