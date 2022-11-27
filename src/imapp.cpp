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
 *    this list of conditions and the following disclaimer.
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

#include <ImApp/imapp.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "fa6.cpp"
#include "roboto.cpp"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// If we found Zlib when running CMake, we define IMAPP_USE_ZLIB, to indicate
// that we should use Zlib to perform compression for PNG images, instead of
// the built-in stb compressor function. This should lead to much smaller
// image sizes, and Zlib is available on most systems.
#ifdef IMAPP_USE_ZLIB
#include <zlib.h>
unsigned char* zlib_compression_for_stbiw(unsigned char* data, int data_len,
                                          int* out_len, int quality) {
  uLongf bufSize = compressBound(data_len);
  // note that buf will be free'd by stb_image_write.h
  // with STBIW_FREE() (plain free() by default)
  unsigned char* buf = reinterpret_cast<unsigned char*>(std::malloc(bufSize));
  if (buf == NULL) return NULL;
  if (compress2(buf, &bufSize, data, data_len, quality) != Z_OK) {
    std::free(buf);
    return NULL;
  }
  *out_len = bufSize;

  return buf;
}
#define STBIW_ZLIB_COMPRESS zlib_compression_for_stbiw
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to
// maximize ease of testing and compatibility with old VS compilers. To link
// with VS2010-era libraries, VS2015+ requires linking with
// legacy_stdio_definitions.lib, which we do using this pragma. Your own project
// should not be affected, as you are likely to link with a newer binary of GLFW
// that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && \
    !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description) {
  std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

namespace ImApp {

App::App(int w, int h, const char* name)
    : window(nullptr), io_(nullptr), style_(nullptr), layers_() {
  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) std::exit(1);

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char* glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  // Create window with graphics context
  window = glfwCreateWindow(w, h, name, NULL, NULL);
  if (window == nullptr) std::exit(1);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);  // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  style_ = &(ImGui::GetStyle());
  io_ = &(ImGui::GetIO());

  // Enable Keyboard Controls
  this->enable_keyboard();

  // TODO should determine these based on DPI
  constexpr float FONT_SIZE = 18.;
  constexpr float ICON_FONT_SIZE = 16.;

  // Load Roboto font
  io_->Fonts->AddFontFromMemoryCompressedTTF(
      RobotoRegular_compressed_data, RobotoRegular_compressed_size, FONT_SIZE);

  // Merge icons from FontAwesome Solid
  static const ImWchar fa_icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
  ImFontConfig fa_icons_config;
  fa_icons_config.MergeMode = true;
  fa_icons_config.PixelSnapH = true;
  fa_icons_config.GlyphMinAdvanceX =
      ICON_FONT_SIZE;  // Make the icon monospaced
  io_->Fonts->AddFontFromMemoryCompressedTTF(
      FASolid_compressed_data, FASolid_compressed_size, ICON_FONT_SIZE,
      &fa_icons_config, fa_icons_ranges);

  // Merge icons from FontAwesome Brands
  static const ImWchar fab_icons_ranges[] = {ICON_MIN_FAB, ICON_MAX_16_FAB, 0};
  ImFontConfig fab_icons_config;
  fab_icons_config.MergeMode = true;
  fab_icons_config.PixelSnapH = true;
  fab_icons_config.GlyphMinAdvanceX =
      ICON_FONT_SIZE;  // Make the icon monospaced
  io_->Fonts->AddFontFromMemoryCompressedTTF(
      FABrands_compressed_data, FABrands_compressed_size, ICON_FONT_SIZE,
      &fab_icons_config, fab_icons_ranges);

  // Setup style
  this->set_default_style();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);
}

App::~App() {
  // Kill all layers first
  for (auto& layer : layers_) layer->on_kill();

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
}

void App::run() {
  // When viewports are enabled we tweak WindowRounding/WindowBg so platform
  // windows can look identical to regular ones. We check this here so that
  // users are ablet to modifiy these flags after construction of the App
  // instance, but before calling run.
  if (io_->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style_->WindowRounding = 0.0f;
    style_->Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application. Generally you may always pass all inputs
    // to dear imgui, and hide them from your application based on those two
    // flags.
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Go through and render all layers
    for (auto& layer : layers_) layer->render();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we
    // save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call
    //  glfwMakeContextCurrent(window) directly)
    if (io_->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow* backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(window);
  }
}

void App::set_icon(const Image& image) {
  // Set program icon
  GLFWimage icon[1];
  icon[0].width = static_cast<int>(image.width());
  icon[0].height = static_cast<int>(image.height());
  icon[0].pixels = const_cast<unsigned char*>(
      reinterpret_cast<const unsigned char*>(&image[0]));
  glfwSetWindowIcon(window, 1, icon);
}

void App::set_default_style() {
  style_->Alpha = 1.0;
  style_->DisabledAlpha = 0.6000000238418579;
  style_->WindowPadding = ImVec2(8.0, 8.0);
  style_->WindowRounding = 0.0;
  style_->WindowBorderSize = 1.0;
  style_->WindowMinSize = ImVec2(32.0, 32.0);
  style_->WindowTitleAlign = ImVec2(0.0, 0.5);
  style_->WindowMenuButtonPosition = ImGuiDir_Left;
  style_->ChildRounding = 0.0;
  style_->ChildBorderSize = 1.0;
  style_->PopupRounding = 0.0;
  style_->PopupBorderSize = 1.0;
  style_->FramePadding = ImVec2(4.0, 3.0);
  style_->FrameRounding = 0.0;
  style_->FrameBorderSize = 0.0;
  style_->ItemSpacing = ImVec2(8.0, 4.0);
  style_->ItemInnerSpacing = ImVec2(4.0, 4.0);
  style_->CellPadding = ImVec2(4.0, 2.0);
  style_->IndentSpacing = 21.0;
  style_->ColumnsMinSpacing = 6.0;
  style_->ScrollbarSize = 14.0;
  style_->ScrollbarRounding = 3.0;  // 0.0;
  style_->GrabMinSize = 10.0;
  style_->GrabRounding = 0.0;
  style_->TabRounding = 3.0;  // 0.0;
  style_->TabBorderSize = 0.0;
  style_->TabMinWidthForCloseButton = 0.0;
  style_->ColorButtonPosition = ImGuiDir_Right;
  style_->ButtonTextAlign = ImVec2(0.5, 0.5);
  style_->SelectableTextAlign = ImVec2(0.0, 0.0);

  ImVec4* Colors = style_->Colors;
  Colors[ImGuiCol_Text] = ImVec4(1.0, 1.0, 1.0, 1.0);
  Colors[ImGuiCol_TextDisabled] =
      ImVec4(0.5921568870544434, 0.5921568870544434, 0.5921568870544434, 1.0);
  Colors[ImGuiCol_WindowBg] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_ChildBg] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_PopupBg] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_Border] =
      ImVec4(0.3058823645114899, 0.3058823645114899, 0.3058823645114899, 1.0);
  Colors[ImGuiCol_BorderShadow] =
      ImVec4(0.3058823645114899, 0.3058823645114899, 0.3058823645114899, 1.0);
  Colors[ImGuiCol_FrameBg] =
      ImVec4(0.2000000029802322, 0.2000000029802322, 0.2156862765550613, 1.0);
  Colors[ImGuiCol_FrameBgHovered] =
      ImVec4(0.1137254908680916, 0.5921568870544434, 0.9254902005195618, 1.0);
  Colors[ImGuiCol_FrameBgActive] =
      ImVec4(0.0, 0.4666666686534882, 0.7843137383460999, 1.0);
  Colors[ImGuiCol_TitleBg] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_TitleBgActive] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_TitleBgCollapsed] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_MenuBarBg] =
      ImVec4(0.2000000029802322, 0.2000000029802322, 0.2156862765550613, 1.0);
  Colors[ImGuiCol_ScrollbarBg] =
      ImVec4(0.2000000029802322, 0.2000000029802322, 0.2156862765550613, 1.0);
  Colors[ImGuiCol_ScrollbarGrab] =
      ImVec4(0.321568638086319, 0.321568638086319, 0.3333333432674408, 1.0);
  Colors[ImGuiCol_ScrollbarGrabHovered] =
      ImVec4(0.3529411852359772, 0.3529411852359772, 0.3725490272045135, 1.0);
  Colors[ImGuiCol_ScrollbarGrabActive] =
      ImVec4(0.3529411852359772, 0.3529411852359772, 0.3725490272045135, 1.0);
  Colors[ImGuiCol_CheckMark] =
      ImVec4(0.0, 0.4666666686534882, 0.7843137383460999, 1.0);
  Colors[ImGuiCol_SliderGrab] =
      ImVec4(0.1137254908680916, 0.5921568870544434, 0.9254902005195618, 1.0);
  Colors[ImGuiCol_SliderGrabActive] =
      ImVec4(0.0, 0.4666666686534882, 0.7843137383460999, 1.0);
  Colors[ImGuiCol_Button] =
      ImVec4(0.2000000029802322, 0.2000000029802322, 0.2156862765550613, 1.0);
  Colors[ImGuiCol_ButtonHovered] =
      ImVec4(0.1137254908680916, 0.5921568870544434, 0.9254902005195618, 1.0);
  Colors[ImGuiCol_ButtonActive] =
      ImVec4(0.1137254908680916, 0.5921568870544434, 0.9254902005195618, 1.0);
  Colors[ImGuiCol_Header] =
      ImVec4(0.2000000029802322, 0.2000000029802322, 0.2156862765550613, 1.0);
  Colors[ImGuiCol_HeaderHovered] =
      ImVec4(0.1137254908680916, 0.5921568870544434, 0.9254902005195618, 1.0);
  Colors[ImGuiCol_HeaderActive] =
      ImVec4(0.0, 0.4666666686534882, 0.7843137383460999, 1.0);
  Colors[ImGuiCol_Separator] =
      ImVec4(0.3058823645114899, 0.3058823645114899, 0.3058823645114899, 1.0);
  Colors[ImGuiCol_SeparatorHovered] =
      ImVec4(0.3058823645114899, 0.3058823645114899, 0.3058823645114899, 1.0);
  Colors[ImGuiCol_SeparatorActive] =
      ImVec4(0.3058823645114899, 0.3058823645114899, 0.3058823645114899, 1.0);
  Colors[ImGuiCol_ResizeGrip] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(0.2000000029802322, 0.2000000029802322, 0.2156862765550613, 1.0);
  Colors[ImGuiCol_ResizeGripActive] =
      ImVec4(0.321568638086319, 0.321568638086319, 0.3333333432674408, 1.0);
  Colors[ImGuiCol_Tab] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_TabHovered] =
      ImVec4(0.1137254908680916, 0.5921568870544434, 0.9254902005195618, 1.0);
  Colors[ImGuiCol_TabActive] =
      ImVec4(0.0, 0.4666666686534882, 0.7843137383460999, 1.0);
  Colors[ImGuiCol_TabUnfocused] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_TabUnfocusedActive] =
      ImVec4(0.0, 0.4666666686534882, 0.7843137383460999, 1.0);
  Colors[ImGuiCol_PlotLines] =
      ImVec4(0.0, 0.4666666686534882, 0.7843137383460999, 1.0);
  Colors[ImGuiCol_PlotLinesHovered] =
      ImVec4(0.1137254908680916, 0.5921568870544434, 0.9254902005195618, 1.0);
  Colors[ImGuiCol_PlotHistogram] =
      ImVec4(0.0, 0.4666666686534882, 0.7843137383460999, 1.0);
  Colors[ImGuiCol_PlotHistogramHovered] =
      ImVec4(0.1137254908680916, 0.5921568870544434, 0.9254902005195618, 1.0);
  Colors[ImGuiCol_TableHeaderBg] =
      ImVec4(0.1882352977991104, 0.1882352977991104, 0.2000000029802322, 1.0);
  Colors[ImGuiCol_TableBorderStrong] =
      ImVec4(0.3098039329051971, 0.3098039329051971, 0.3490196168422699, 1.0);
  Colors[ImGuiCol_TableBorderLight] =
      ImVec4(0.2274509817361832, 0.2274509817361832, 0.2470588237047195, 1.0);
  Colors[ImGuiCol_TableRowBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
  Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0, 1.0, 1.0, 0.05999999865889549);
  Colors[ImGuiCol_TextSelectedBg] =
      ImVec4(0.0, 0.4666666686534882, 0.7843137383460999, 1.0);
  Colors[ImGuiCol_DragDropTarget] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_NavHighlight] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
  Colors[ImGuiCol_NavWindowingHighlight] =
      ImVec4(1.0, 1.0, 1.0, 0.699999988079071);
  Colors[ImGuiCol_NavWindowingDimBg] =
      ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929,
             0.2000000029802322);
  Colors[ImGuiCol_ModalWindowDimBg] =
      ImVec4(0.1450980454683304, 0.1450980454683304, 0.1490196138620377, 1.0);
}

Image Image::from_file(const std::filesystem::path& fname) {
  int img_width = 0;
  int img_height = 0;

  // Make sure file exists
  if (std::filesystem::exists(fname) == false) {
    std::string mssg = "ImApp::Image::from_file: File with name \"";
    mssg += fname.string() + "\" does not exist.\n";
    throw std::runtime_error(mssg);
  }

  // Get image data since file exists
  unsigned char* data =
      stbi_load(fname.c_str(), &img_width, &img_height, NULL, 4);

  if (data == NULL) {
    // Loading failed. Throw exception.
    std::string mssg = "ImApp::Image::from_file: stbi_load failure.\n";
    mssg += "stbi_failure_reason: ";
    mssg += std::string(stbi_failure_reason());
    mssg += "\n";
    throw std::runtime_error(mssg);
  }

  const std::uint32_t width = static_cast<std::uint32_t>(img_width);
  const std::uint32_t height = static_cast<std::uint32_t>(img_height);
  Pixel* pdata = reinterpret_cast<Pixel*>(data);

  // Create image
  Image img(height, width);

  // Set all pixels
  for (std::size_t i = 0; i < img.size(); i++) {
    img[i] = pdata[i];
  }

  // Free stb_image data
  stbi_image_free(data);

  return img;
}

bool Image::save_png(const std::filesystem::path& fname) {
  const unsigned char* data = reinterpret_cast<unsigned char*>(image_.data());
  const int width = static_cast<int>(this->width());
  const int height = static_cast<int>(this->height());
  const int stride = 4 * width;
  const int err = stbi_write_png(fname.c_str(), width, height, 4, data, stride);
  return err != 0;
}

bool Image::save_jpg(const std::filesystem::path& fname) {
  const unsigned char* data = reinterpret_cast<unsigned char*>(image_.data());
  const int width = static_cast<int>(this->width());
  const int height = static_cast<int>(this->height());
  const int err = stbi_write_jpg(fname.c_str(), width, height, 4, data, 100);
  return err != 0;
}

void Image::send_to_gpu() {
  if (ogl_texture_id_) {
    // Texture already exists on GPU. We just need to update it.
    glBindTexture(GL_TEXTURE_2D, ogl_texture_id_.value());

    // For some reason it can't seem to find glTexSubImage2D, so for now I
    // will just use glTexImage2D when updating the image.
    // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, 0, GL_RGBA,
    // GL_UNSIGNED_BYTE, image_.data());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, image_.data());
  } else {
    // Texture not on GPU yet. Need to do everything from scratch.

    // Create a OpenGL texture identifier
    std::uint32_t texture_id = 0;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // The ImGui site said I should need these two lines for WebGL, but
    // they weren't found by my loader so we will ignore them.
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, image_.data());

    // Save texture id to object
    ogl_texture_id_ = texture_id;
  }
}

void Image::delete_from_gpu() {
  if (ogl_texture_id_) {
    glDeleteTextures(1, &ogl_texture_id_.value());
    ogl_texture_id_ = std::nullopt;
  }
}

}  // namespace ImApp
