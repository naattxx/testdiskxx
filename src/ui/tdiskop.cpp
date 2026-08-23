#include "tdiskop.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"
#include "src/common.hpp"
#include "toptions.hpp"
#include <config.h>
#include <format>
#include <string_view>

using namespace ftxui;

void menu_disk(disk_t &disk, const int verbose, bool dump, const int save_header)
{
  bool align  = true;
  bool expert = false;

  auto screen = App::Fullscreen();
  Component root;

  ButtonOption buttonOptions = ButtonOption();
  buttonOptions.transform    = [](const EntryState &s) -> Element {
    if (s.focused)
    {
      return hflow({text("> " + s.label.substr(0, s.label.find(']') + 1)) |
                        bgcolor(Color::White) | color(Color::Black) | bold,
                    text(s.label.substr(s.label.find(']') + 1))});
    }

    return text("  " + s.label);
  };
  auto options = Container::Vertical({
      Button(
          "[ Analyse  ] Analyse current partition structure and search for "
          "lost "
          "partitions",
          []() {}, buttonOptions
      ),
      Button(
          "[ Advanced ] Filesystem Utils", []() {}, buttonOptions
      ),
      Button(
          "[ Geometry ] Change disk geometry", []() {}, buttonOptions
      ),
      Button(
          "[ Options  ] Modify options",
          [&]() { interface_options(root, dump, align, expert); }, buttonOptions
      ),
      Button("[ Quit     ] Return to disk selection", screen.ExitLoopClosure(),
             buttonOptions),
  });
  root         = Renderer(options, [&]() {
    return vbox({
        hflow({text("TestDisk++ "), bold(text(VERSION)),
               text(", Data Recovery Utility, "), text(TESTDISKDATE)}),
        text("naattxx"),
        text("https://github.com/naattxx/testdiskxx"),
        separatorEmpty(),
        text(disk.description_short(disk)),
        hflow({(disk.geom.heads_per_cylinder == 1 &&
                disk.geom.sectors_per_head == 1)
                   ? text(std::format("     {} sectors",
                                      disk.disk_size / disk.sector_size))
                   : text(std::format("     CHS {} {} {}", disk.geom.cylinders,
                                      disk.geom.heads_per_cylinder,
                                      disk.geom.sectors_per_head)),
               text(std::format(" - sector size={}", disk.sector_size))}),
        separatorEmpty(),
        options->Render(),
        filler(),
        paragraph("Note: Correct disk geometry is required for a successful "
                  "recovery. 'Analyse'"
                  " process may give some warnings if it thinks the logical "
                  "geometry is mismatched."),
    });
  });

  screen.Loop(root);
}
