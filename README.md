# ImApp - A C++ Boiler Plate Library for Dear ImGui

ImApp is a small library which builds upon Dear ImGui, taking care of the
boiler plate required to setup a window and application environment. This
greatly streamlines the process of creating a simple GUI program in C++,
which should be easily portable to other systems. In addition to shipping
with ImGui, ImPlot is also provided, making it easy to generate plots and
graphs, for those with more “scientific” interests in mind. Thanks to stb, it
is easy to read/write images from/to a file, and then display them in your
application. During the build process, ImApp will automatically download and
compile GLFW, which provides the interface to generate the application window,
and interact with the operating system.

The library was somewhat inspired by the
[Walnut](https://github.com/TheCherno/Walnut) project, but there are a few
important differences. The first is that Walnut uses Vulkan as a graphics back
end, while ImApp uses OpenGL. Vulkan is a great choice is you are looking to do
some serious graphics/games programming, but is very complicated and difficult
to learn. OpenGL is much easier to learn and use, making it a better choice for
those who want to make simple application without the need of high performance
graphics.

ImApp currently ships with the following libraries:
 * [ImGui](https://github.com/ocornut/imgui) v1.92.4-docking
 * [ImPlot](https://github.com/epezent/implot) 81b8b19 (just after v0.17)
 * [ImPlot3D](https://github.com/brenocq/implot3d) currently on my personal fork, fix/unused_warning branch 
 * [GLFW](https://github.com/glfw/glfw) v3.3.10
 * [stb\_image](https://github.com/nothings/stb) v2.30
 * [stb\_image\_write](https://github.com/nothings/stb) v1.16
 * [FontAwesome](https://github.com/FortAwesome/Font-Awesome) v6.2.0
 * [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders) 685673d
