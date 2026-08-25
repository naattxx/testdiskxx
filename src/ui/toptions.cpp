#include "toptions.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"
#include "src/log.hpp"

using namespace ftxui;

void interface_options(const ftxui::Component root, bool &dump, bool &align,
                       bool &expert)
{
  ButtonOption buttonOptions = ButtonOption();
  buttonOptions.transform    = [](const EntryState &s) -> Element {
    if (s.focused)
    {
      return hflow({text("  "),
                    text("> " + s.label.substr(0, s.label.find(']') + 1)) |
                        bgcolor(Color::White) | color(Color::Black) | bold,
                    text(s.label.substr(s.label.find(']') + 1))});
    }

    return text("    " + s.label);
  };
  bool show_modal = true;
  auto screen     = App::Fullscreen();
  auto options    = Container::Vertical(
      {Checkbox("Expert mode - Expert mode adds some functionalities", &expert),
       Checkbox(
           "Align partition - Align partitions to cylinder or 1MiB boundaries",
           &align
       ),
       Checkbox("Dump - Dump essential sectors", &dump),
       Button("[ Ok ]", screen.ExitLoopClosure(), buttonOptions)}
  );
  auto dialog =
      Renderer(options,
               [&]() -> Element { return vbox({options->Render()}); }) |
      size(WIDTH, GREATER_THAN, 20) | border | center;
  screen.Loop(root | Modal(dialog, &show_modal));

  /* write new options to log file */
  log_info("New options :");
  log_info(" Dump : {}", dump ? "Yes" : "No");
  log_info(" Align partition: {}", align ? "Yes" : "No");
  log_info(" Expert mode : {}", expert ? "Yes" : "No");
}
