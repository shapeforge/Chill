#pragma once

class SelectableUI;

// ProcessingGraph.h
namespace Chill
{
  class ProcessingGraph;
}


#include <LibSL\LibSL.h>
#include <LibSL\LibSL_gl.h>

#include "imgui\imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS true
#include "imgui\imgui_internal.h"

#include "Style.h"



class UI
{
public:
  bool m_visible = true;

  /** Component's size */
  ImVec2 m_size     = ImVec2(0.0f, 0.0f);

  /** Component's position */
  ImVec2 m_position = ImVec2(0.0f, 0.0f);

  /** Graphical scale */
  float m_scale     = 1.0f;

public:
  Style style;

  /**
   * Draw the component
   * return true if the element is rendered, else false
   */
  virtual bool draw() = 0;

  /**
   *  Resize a component (scale independant)
   *  @param _size The new component size
   */
  void resize(ImVec2 _size) {
    m_size = _size;
  }

  /**
   *  Move a component
   *  @param _position The new position
   */
  void setPosition(ImVec2 _position) {
    m_position = _position;
  }

  /**
  *  Translate a component
  *  @param _delta The delta
  */
  void translate(ImVec2 _delta) {
    m_position += _delta;
  }

  /**
   *  Set the scale of a component
   *  param _scale The new scale
   */
  void setScale(const float _scale) {
    m_scale = _scale;
  }

  inline const ImVec2 getPosition() {
    return m_position;
  }

  inline const float getScale() {
    return m_scale;
  }

  inline const int64_t getUniqueID() {
    return int64_t(this);
  }
};

class SelectableUI : public UI
{
public:
  bool m_selected;
  bool m_edit;
  
  /** Display name. */
  std::string m_name;

  /** Display color. */
  ImU32 m_color;

  /** Parent graph, raw pointer is needed. */
  Chill::ProcessingGraph * m_owner = NULL;



  SelectableUI() {
    m_selected = false;
    m_edit = false;
  }

  SelectableUI(SelectableUI &_copy) {
    m_selected = false;
    m_edit = false;
  }


  /**
   *  Get the name of this ui element.
   *  @return The name of the  ui element.
   **/
  inline const std::string name() {
    return m_name;
  }

  /**
   *  Set the name of this ui element.
   *  @param _name The color of the ui element.
   **/
  void setName(const std::string& _name) {
    m_name = _name;
  }

  /**
   *  Get the color of this ui element.
   *  @return The color of the ui element.
   **/
  inline const ImU32 color() {
    return m_color;
  }

  /**
   *  Set the color of ui element.
   *  @param _color The color of the ui element.
   **/
  void setColor(const ImU32& _color) {
    m_color = _color;
  }

  bool draw() { return true; };

  virtual AutoPtr<SelectableUI> clone() = 0;

  /**
  *  Set a new owner.
  *  @param _owner The graph that contains this processor.
  */
  void setOwner(Chill::ProcessingGraph * _owner) {
    m_owner = _owner;
  }

  /**
  *  Get the owner of this processor.
  *  @return The parent graph.
  */
  Chill::ProcessingGraph * owner() {
    return m_owner;
  }

};