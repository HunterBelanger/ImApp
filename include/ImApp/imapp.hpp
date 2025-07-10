/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2022, Hunter Belanger
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this List of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * */
#ifndef IMAPP_H
#define IMAPP_H

#include <ImApp/IconsFontAwesome6.h>
#include <ImApp/IconsFontAwesome6Brands.h>
#include <ImApp/imgui.h>
#include <ImApp/implot.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

struct GLFWwindow;

namespace ImApp {

/**
 * @brief A class which represents a single pixel in an image. Each pixel has
 * four 8-bit channels: Red, Green, Blue, and Alpha.
 */
class Pixel {
 public:
  Pixel() : r_(255), g_(255), b_(255), a_(255) {}

  /**
   * @breif Constructs a pixel with a specified color and opacity.
   * @param R Value of the red channel in [0,255].
   * @param G Value of the green channel in [0,255].
   * @param B Value of the blue channel in [0,255].
   * @param A Value of alpha (the opacity) in [0,255].
   */
  Pixel(std::uint8_t R, std::uint8_t G, std::uint8_t B, std::uint8_t A = 255)
      : r_(R), g_(G), b_(B), a_(A) {}

  /**
   * @brief Returns a modifiable reference to the red channel.
   */
  std::uint8_t& r() { return r_; }

  /**
   * @brief Returns a const reference to the red channel.
   */
  const std::uint8_t& r() const { return r_; }

  /**
   * @brief Returns a modifiable reference to the green channel.
   */
  std::uint8_t& g() { return g_; }

  /**
   * @brief Returns a const reference to the green channel.
   */
  const std::uint8_t& g() const { return g_; }

  /**
   * @brief Returns a modifiable reference to the blue channel.
   */
  std::uint8_t& b() { return b_; }

  /**
   * @brief Returns a const reference to the blue channel.
   */
  const std::uint8_t& b() const { return b_; }

  /**
   * @brief Returns a modifiable reference to the alpha channel.
   */
  std::uint8_t& a() { return a_; }

  /**
   * @brief Returns a const reference to the alpha channel.
   */
  const std::uint8_t& a() const { return a_; }

 private:
  std::uint8_t r_, g_, b_, a_;
};

/**
 * @brief A class which respresents an image, containing an array of Pixel
 * objects. All of the Pixels are stored in row major order.
 */
class Image {
 public:
  /**
   * @breif Create an image of specified height and width.
   * @param height Initial height of the image.
   * @param width Initial width of the image.
   */
  Image(std::uint32_t height, std::uint32_t width)
      : height_(height),
        width_(width),
        image_(height_ * width_, Pixel()),
        ogl_texture_id_(std::nullopt) {}

  ~Image() { this->delete_from_gpu(); }

  /**
   * @brief Loads an Image from a file. Can be almost any common image type.
   * @brief fname Path to the file containing the image.
   */
  static Image from_file(const std::filesystem::path& fname);

  /**
   * @brief Saves the image in a PNG file.
   * @param fname Path to the file where the image will be written.
   */
  bool save_png(const std::filesystem::path& fname);

  /**
   * @brief Saves the image in a JPG file.
   * @param fname Path to the file where the image will be written.
   */
  bool save_jpg(const std::filesystem::path& fname);

  /**
   * @brief Returns the width of the image.
   */
  const std::uint32_t& width() const { return width_; }

  /**
   * @brief Returns the height of the image.
   */
  const std::uint32_t& height() const { return height_; }

  /**
   * @breif Returns a modifiable reference to a Pixel, given a linear index.
   * @param i Linear index into pixel vector.
   */
  Pixel& operator[](std::size_t i) { return image_[i]; }

  /**
   * @breif Returns a const reference to a Pixel, given a linear index.
   * @param i Linear index into pixel vector.
   */
  const Pixel& operator[](std::size_t i) const { return image_[i]; }

  /**
   * @breif Returns the linear size of the pixel buffer (i.e. width * height).
   */
  std::uint32_t size() const { return width_ * height_; }

  /**
   * @breif Returns a modifiable reference to a Pixel for a given row and
   * column.
   * @param h Index for the row of the pixel. Must be in the interval
   * [0,height).
   * @param w Index for the column of the pixel. Must be in the interval
   * [0,width).
   */
  Pixel& operator()(std::uint32_t h, std::uint32_t w) {
    std::size_t i = w + (h * width_);
    return image_[i];
  }

  /**
   * @breif Returns a const reference to a Pixel for a given row and column.
   * @param h Index for the row of the pixel. Must be in the interval
   * [0,height).
   * @param w Index for the column of the pixel. Must be in the interval
   * [0,width).
   */
  const Pixel& operator()(std::uint32_t h, std::uint32_t w) const {
    std::size_t i = w + (h * width_);
    return image_[i];
  }

  /**
   * @breif Returns a modifiable reference to a Pixel for a given row and
   * column. An std::out_of_range exception is thrown if the desired row or
   * column are out of range.
   * @param h Index for the row of the pixel. Must be in the interval
   * [0,height).
   * @param w Index for the column of the pixel. Must be in the interval
   * [0,width).
   */
  Pixel& at(std::uint32_t h, std::uint32_t w) {
    if (h >= height_) {
      throw std::out_of_range("ImApp::Image::at: h must be < height.");
    } else if (w >= width_) {
      throw std::out_of_range("ImApp::Image::at: w must be < width.");
    }

    std::size_t i = w + (h * width_);
    return image_[i];
  }

  /**
   * @breif Returns a const reference to a Pixel for a given row and
   * column. An std::out_of_range exception is thrown if the desired row or
   * column are out of range.
   * @param h Index for the row of the pixel. Must be in the interval
   * [0,height).
   * @param w Index for the column of the pixel. Must be in the interval
   * [0,width).
   */
  const Pixel& at(std::uint32_t h, std::uint32_t w) const {
    if (h >= height_) {
      throw std::out_of_range("ImApp::Image::at: h must be < height.");
    } else if (w >= width_) {
      throw std::out_of_range("ImApp::Image::at: w must be < width.");
    }

    std::size_t i = w + (h * width_);
    return image_[i];
  }

  /**
   * @breif Resizes the image. None of the pixels are modified in the image upon
   * resize. If an image is made larger, the new pixels will be white.
   * @param height New image height.
   * @param width New image width.
   */
  void resize(std::uint32_t height, std::uint32_t width) {
    height_ = height;
    width_ = width;
    image_.resize(height_ * width_);
  }

  /**
   * @brief Sends the image to the GPU, and populates the OpenGL texture id, if
   * it is not yet populated. This method is also used to update the image on
   * the GPU if it has been modified.
   */
  void send_to_gpu();

  /**
   * @breif Removes the image from the GPU, and clears the OpenGL texture id.
   * This method is automatically called on destruction.
   */
  void delete_from_gpu();

  /**
   * @breif Returns true if the image is on, and false otherwise. This does NOT
   * mean that the version of the image on the GPU is up-to-date with the image
   * stored in the object.
   */
  bool on_gpu() const { return ogl_texture_id_.has_value(); }

  /**
   * @breif Returns the optional texture ID for the image on the GPU, which is
   * returned as an std::uint32_t.
   */
  const std::optional<std::uint32_t>& ogl_texture_id() const {
    return ogl_texture_id_;
  }

 private:
  std::uint32_t height_, width_;
  std::vector<Pixel> image_;
  std::optional<std::uint32_t> ogl_texture_id_;

  Image() : height_(0), width_(0), image_(), ogl_texture_id_(std::nullopt) {}
};

// Forward declare ImApp::App class to store pointer in Layer.
class App;

/**
 * @breif Abstract class for the interface of a rendering layer.
 */
class Layer {
 public:
  Layer() : app_(nullptr) {}
  virtual ~Layer() = default;

  /**
   * @breif A virtual method which is called when a Layer is added to an App.
   */
  virtual void on_push(){};

  /**
   * @breif A virtual method which is called for each frame in the rendering
   * pipeline.
   */
  virtual void render(){};

  /**
   * @breif A virtual method which is called on all Layer instances in an App,
   * when the App is destroyed, or is closed.
   */
  virtual void on_kill(){};

  /**
   * @brief Returns a pointer to the associated ImApp::App instance. This
   * pointer will only be set and valid if the layer has been pushed onto
   * an ImApp::App instance.
   */
  App* app() const { return app_; }

 private:
  App* app_;

  void set_app_ptr(App* app_ptr) { app_ = app_ptr; }

  friend ImApp::App;
};

/**
 * @brief This class is used to build a graphical user interface (GUI) for a
 * user application. It first initializes the window, and then draws in the
 * window by iterating through a stack of Layer objects, drawing each one in
 * sequence.
 */
class App {
 public:
  /**
   * @brief Builds the application window with a width, height, and window name.
   * @param w Initial window width.
   * @param h Initial window height.
   * @param name Application or window name.
   */
  App(int w, int h, const char* name);
  ~App();

  /**
   * @brief Sets the application icon.
   * @param image Reference to the Image containing the icon.
   */
  void set_icon(const Image& image);

  /**
   * @brief Starts the application loop. All layers must be added before calling
   * this method.
   */
  void run();

  /**
   * @breif Adds a layer to the application rendering stack.
   * @param layer Pointer to the new layer to be added to the rendering stack.
   */
  void push_layer(std::unique_ptr<Layer> layer) {
    layers_.push_back(std::move(layer));
    layers_.back()->on_push();
    layers_.back()->set_app_ptr(this);
  }

  /**
   * @brief Returns a reference to the current application sytle settings.
   */
  ImGuiStyle& style() { return *style_; }

  /**
   * @breif Returns a reference to the current application IO settings.
   */
  ImGuiIO& io() { return *io_; }

  /**
   * @brief Enables the docking capabilities of ImGUI if disabled. Docking is
   * disabled by default.
   */
  void enable_docking() { io_->ConfigFlags |= ImGuiConfigFlags_DockingEnable; }

  /**
   * @brief Disables the docking capabilities of ImGUI if enabled.
   */
  void disable_docking() {
    io_->ConfigFlags &= ~(ImGuiConfigFlags_DockingEnable);
  }

  /**
   * @brief Enables the viewports capabilities of ImGUI if disabled. Viewports
   * are disabled by default.
   */
  void enable_viewports() {
    io_->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  }

  /**
   * @brief Disables the viewports capabilities of ImGUI if enabled.
   */
  void disable_viewports() {
    io_->ConfigFlags &= ~(ImGuiConfigFlags_ViewportsEnable);
  }

  /**
   * @brief Enables the gamepad if disabled. The gamepad is disabled by default.
   */
  void enable_gamepad() {
    io_->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  }

  /**
   * @brief Disables the gamepad if enabled.
   */
  void disable_gamepad() {
    io_->ConfigFlags &= ~(ImGuiConfigFlags_NavEnableGamepad);
  }

  /**
   * @brief Enables the keyboard if disabled. The keyboard is enabled by
   * default.
   */
  void enable_keyboard() {
    io_->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  }

  /**
   * @brief Disables the keyboard if enabled.
   */
  void disable_keyboard() {
    io_->ConfigFlags &= ~(ImGuiConfigFlags_NavEnableKeyboard);
  }

  /**
   * @brief Applies the default application style.
   */
  void set_default_style();

 private:
  GLFWwindow* window;
  ImGuiIO* io_;
  ImGuiStyle* style_;
  std::vector<std::unique_ptr<Layer>> layers_;
};

}  // namespace ImApp
#endif
