#include "intrfn.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/terminal.hpp"
#include <string_view>

using namespace ftxui;

auto ask_confirmation(std::string_view msg) -> bool
{
  auto screen    = App::Fullscreen();
  auto result    = false;
  auto yesButton = Button(
      "Yes",
      [&] -> void {
        result = true;
        screen.Exit();
      },
      ButtonOption::Ascii()
  );
  auto noButton = Button("No", screen.ExitLoopClosure(), ButtonOption::Ascii());

  auto container = Container::Vertical({noButton, yesButton});
  auto dialog    = Renderer(container, [&] -> Element {
    return vbox({
               paragraph(msg),
               separator(),
               container->Render(),
           }) |
           size(WIDTH, ftxui::LESS_THAN, Terminal::Size().dimx * 0.75f) |
           borderHeavy | center;
  });

  screen.Loop(dialog);
  return result;
}

void display_message(const Component &root, std::string_view msg)
{
  auto screen = App::Fullscreen();

  auto okButton = Button("Ok", screen.ExitLoopClosure(), ButtonOption::Ascii());

  auto dialog = Renderer(okButton, [&] -> Element {
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
