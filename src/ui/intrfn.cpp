#include "intrfn.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/terminal.hpp"
#include <cerrno>
#include <cstring>
#include <format>
#include <string>
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

auto ask_log_location(const std::string &failed_filename) -> std::string
{
  auto screen = App::Fullscreen();

  std::string content;
  auto input = Input(&content, failed_filename,
                     {
                         .multiline = false,
                         .on_enter  = screen.ExitLoopClosure(),
                     });

  auto dialog = Renderer(input, [&] -> Element {
    return vbox({
               !failed_filename.empty()
                   ? text(std::format("Cannot open {}: {}", failed_filename,
                                      std::strerror(errno)))
                   : emptyElement(),
               hflow({
                   text("Please enter the full log filename or press "),
                   text("Enter") | bold,
                   text("to abort log file creation."),
               }),
               separator(),
               input->Render(),
           }) |
           size(WIDTH, ftxui::LESS_THAN, Terminal::Size().dimx * 0.75f) |
           borderHeavy | center;
  });

  screen.Loop(dialog);

  return content;
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
