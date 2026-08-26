#include "intrfn.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/terminal.hpp"
#include <string_view>

using namespace ftxui;

void display_message(const Component root, std::string_view msg)
{
  auto screen = App::Fullscreen();

  auto okButton =
      Button("Ok", screen.ExitLoopClosure(), ButtonOption::Ascii());

  auto dialog = Renderer(okButton, [&]() -> Element {
    return vbox({
               paragraph(msg),
               separator(),
               okButton->Render(),
           }) |
           size(WIDTH, ftxui::LESS_THAN, Terminal::Size().dimx * 0.75f) |
           borderHeavy | center;
  });

  bool show_modal = true;
  screen.Loop(root | Modal(dialog, &show_modal));
}
