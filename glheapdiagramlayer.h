#ifndef GLHEAPDIAGRAMLAYER_H
#define GLHEAPDIAGRAMLAYER_H

#include <memory>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>

#include "displayheapwindow.h"
#include "vertex.h"

// A "layer" -- a collection of bits of data, vertices, shaders etc. that
// allows easy drawing of an extra layer of the heap diagram.
class GLHeapDiagramLayer
{
public:
  GLHeapDiagramLayer(const std::string &vertex_shader_name, const std::string &fragment_shader_name);
  ~GLHeapDiagramLayer();
  void initializeGLStructures(QOpenGLFunctions* parent);
  void paintLayer(ivec2 minimum_tick, ivec3 maximum_tick, const QMatrix2x2& heap_to_screen);

  // Get a pointer to the HeapVertex vector so it can be filled.
  std::vector<HeapVertex>* getVertexVector() { return &layer_vertices_; }
private:
  void setupStandardUniforms();
  // Helper functions to set the uniforms for the shaders.
  void setTickBaseUniforms(int32_t x, int32_t y);
  void setHeapBaseUniforms(int32_t x, int32_t y, int32_t z);
  void setHeapToScreenMatrix(const QMatrix2x2& heap_to_screen);

  std::string vertex_shader_name_;
  std::string fragment_shader_name_;
  bool is_initialized_ = false;

  // The vertices for this layer.
  std::vector<HeapVertex> layer_vertices_;

  // An ivec3 that is filled with the uint64_t of the base address of the
  // displayed fraction of the heap.
  int uniform_visible_heap_base_A_;
  int uniform_visible_heap_base_B_;
  int uniform_visible_heap_base_C_;

  // The minimum tick is provided as ivec2.
  int uniform_visible_tick_base_A_;
  int uniform_visible_tick_base_B_;

  // The matrix to project heap vertices to the screen.
  int uniform_vertex_to_screen_;

  // VAO, VBO and shader for this layer.
  QOpenGLBuffer layer_vertex_buffer_;
  QOpenGLVertexArrayObject layer_vao_;
  std::unique_ptr<QOpenGLShaderProgram> layer_shader_program_;

};

#endif // GLHEAPDIAGRAMLAYER_H
