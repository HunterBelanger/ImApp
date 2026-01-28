#include <ImApp/imapp.hpp>

class DemoLayer : public ImApp::Layer {
  public:
    DemoLayer() {}

    void render() {
      ImGui::ShowDemoWindow();

      ImPlot::ShowDemoWindow();

      ImPlot3D::ShowDemoWindow();
    }
};

int main() {
  ImApp::App app(1080, 1080, "ImApp Demo");
  app.push_layer(std::make_unique<DemoLayer>());
  app.run();

  return 0;
}
