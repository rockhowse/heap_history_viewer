#include <math.h>

#include <iostream>
#include <fstream>

#include <QApplication>
#include <QtGlobal>
#include <QOpenGLShaderProgram>
#include <QMouseEvent>
#include <QWindow>

#include "heapblock.h"
#include "vertex.h"
#include "glheapdiagram.h"

GLHeapDiagram::GLHeapDiagram(QWidget *parent)
    : QOpenGLWidget(parent), block_layer_(new GLHeapDiagramLayer(":/simple.vert", ":/simple.frag")) {

  //  QObject::connect(this, SIGNAL(blockClicked), parent->parent(),
  //  SLOT(blockClicked));
}

void GLHeapDiagram::initializeGL() {
  initializeOpenGLFunctions();
  glEnable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

  // Load the heap history.
  std::ifstream ifs("/tmp/heap.json", std::fstream::in);
  heap_history_.LoadFromJSONStream(ifs);
  heap_history_.setCurrentWindowToGlobal();

  // Initialize the layers.
  std::vector<HeapVertex>* block_vertices = block_layer_->getVertexVector();
  heap_history_.heapBlockVerticesForActiveWindow(block_vertices);
  block_layer_->initializeGLStructures(this);
  //setupHeapblockGLStructures();
  //setupGridGLStructures();
}

QSize GLHeapDiagram::minimumSizeHint() { return QSize(500, 500); }

QSize GLHeapDiagram::sizeHint() { return QSize(1024, 1024); }

void GLHeapDiagram::updateHeapToScreenMap() {
  double y_scaling;
  double x_scaling;
  getScaleFromHeapToScreen(&x_scaling, &y_scaling);

  heap_to_screen_matrix_.data()[0] = x_scaling;
  heap_to_screen_matrix_.data()[3] = y_scaling;
}

void GLHeapDiagram::getScaleFromHeapToScreen(double *scale_x, double *scale_y) {
  long double y_scaling =
      heap_history_.getCurrentWindow().getYScalingHeapToScreen();
  long double x_scaling =
      heap_history_.getCurrentWindow().getXScalingHeapToScreen();
  *scale_x = x_scaling;
  *scale_y = y_scaling;
}

bool GLHeapDiagram::screenToHeap(double x, double y, uint32_t *tick,
                                 uint64_t *address) {
  return heap_history_.getCurrentWindow().mapDisplayCoordinateToHeap(x, y, tick,
                                                                     address);
}

void GLHeapDiagram::debugDumpVerticesAndMappings() {
  //heap_history_.getCurrentWindow().setDebug(true);
  /*
  int index = 0;
  for (const HeapVertex &vertex : g_vertices) {
    std::pair<float, float> vertex_mapped =
        heap_history_.getCurrentWindow().mapHeapCoordinateToDisplay(
            vertex.getX(), vertex.getY());

    if ((vertex_mapped.first >= -1.0) && (vertex_mapped.second <= 1.0) &&
        (vertex_mapped.second >= -1.0) && (vertex_mapped.second <= 1.0)) {
            printf("[Debug][Normal] Vertex %d at %d, %lx -> %f %f\n", index,
  vertex.getX(), vertex.getY(),
                   vertex_mapped.first, vertex_mapped.second);
      } else {
             printf("[Debug][Weird!] Vertex %d at %d, %lx -> %f %f\n", index,
  vertex.getX(), vertex.getY(),
                   vertex_mapped.first, vertex_mapped.second);
      }
    ++index;
  }
  printf("[Debug] ----\n");
  fflush(stdout);*/
}

void GLHeapDiagram::paintGL() {
  glClear(GL_COLOR_BUFFER_BIT);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  updateHeapToScreenMap();
  debugDumpVerticesAndMappings();
  const DisplayHeapWindow& heap_window = heap_history_.getCurrentWindow();
  block_layer_->paintLayer(
    heap_window.getMinimumTick(),
    heap_window.getMinimumAddress(),
    heap_to_screen_matrix_);
  /*
    printf("height is %lx, width is %lx\n",
    heap_history_.getCurrentWindow().height(),
    heap_history_.getCurrentWindow().width());
    printf("minimum_address is %lx, maximum_address is %lx\n",
    heap_history_.getCurrentWindow().getMinimumAddress(),
    heap_history_.getCurrentWindow().getMaximumAddress());
    for (const HeapVertex &vertex : g_vertices) {
      double x, y;
      heapToScreen(vertex.getX(), vertex.getY(), &x, &y);
      printf("[!] Vertex is %lx, %lx -> %f %f\n", vertex.getX(), vertex.getY(),
             x, y);
    }
    fflush(stdout);
  */

  /*
  // Render the grid.
  updateUnitSquareToHeapMap();
  grid_shader_->bind();
  grid_shader_->setUniformValue(uniform_heap_to_screen_map_,
                                heap_to_screen_matrix_);
  grid_shader_->setUniformValue(uniform_grid_to_heap_translation_,
                                unit_square_to_heap_translation_);
  grid_shader_->setUniformValue(uniform_grid_to_heap_map_,
                                unit_square_to_heap_matrix_);
  grid_shader_->setUniformValue(uniform_heap_to_screen_translation_,
                                heap_to_screen_translation_);
  {
    grid_vao_.bind();
    glDrawArrays(GL_LINES, 0, g_grid_vertices.size() * sizeof(HeapVertex));
    grid_vao_.release();
  }
  grid_shader_->release();*/
}

void GLHeapDiagram::update() {
  printf("Update called\n");
  QOpenGLWidget::update();
}

void GLHeapDiagram::resizeGL(int w, int h) { printf("Resize GL was called\n"); }

GLHeapDiagram::~GLHeapDiagram() {
}

void GLHeapDiagram::mousePressEvent(QMouseEvent *event) {
  double x = static_cast<double>(event->x()) / this->width();
  double y = static_cast<double>(event->y()) / this->height();
  uint32_t tick;
  uint64_t address;
  last_mouse_position_ = event->pos();
  if (!screenToHeap(x, y, &tick, &address)) {
    emit showMessage("Click out of bounds.");
    return;
  }
  HeapBlock current_block;
  uint32_t index;

  printf("clicked at tick %d and address %lx\n", tick, address);
  fflush(stdout);

  if (!heap_history_.getBlockAtSlow(address, tick, &current_block, &index)) {

    emit showMessage("Nothing here.");
  } else {
    emit blockClicked(true, current_block);
  }
}

void GLHeapDiagram::mouseMoveEvent(QMouseEvent *event) {
  double dx = event->x() - last_mouse_position_.x();
  double dy = event->y() - last_mouse_position_.y();

  if (event->buttons() & Qt::LeftButton) {
    heap_history_.panCurrentWindow(dx / this->width(), dy / this->height());

    QOpenGLWidget::update();
  } else if (event->buttons() & Qt::RightButton) {
    QOpenGLWidget::update();
  }

  last_mouse_position_ = event->pos();
}

void GLHeapDiagram::wheelEvent(QWheelEvent *event) {
  float movement_quantity = event->angleDelta().y() / 500.0;
  double how_much_y = 1.0;
  double how_much_x = 1.0;
  Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
  double point_x = static_cast<double>(event->x()) / this->width();
  double point_y = static_cast<double>(event->y()) / this->height();
  long double max_height =
      (heap_history_.getMaximumAddress() - heap_history_.getMinimumAddress()) *
      1.5 * 16;
  long double max_width =
      (heap_history_.getMaximumTick() - heap_history_.getMinimumTick()) * 1.5 * 16;

  if (!(modifiers & Qt::ControlModifier) && (modifiers & Qt::ShiftModifier)) {
    how_much_y = 1.0 - movement_quantity;
    heap_history_.zoomToPoint(point_x, point_y, how_much_x, how_much_y,
                              max_height, max_width);
  } else if ((modifiers & Qt::ControlModifier) &&
             (modifiers & Qt::ShiftModifier)) {
    how_much_x = 1.0 - movement_quantity;
    heap_history_.zoomToPoint(point_x, point_y, how_much_x, how_much_y,
                              max_height, max_width);
  } else if (modifiers & Qt::ControlModifier) {
    how_much_y = 1.0 - movement_quantity;
    how_much_x = 1.0 - movement_quantity;
    heap_history_.zoomToPoint(point_x, point_y, how_much_x, how_much_y,
                              max_height, max_width);
  }

  QOpenGLWidget::update();
}
