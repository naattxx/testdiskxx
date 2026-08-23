#include "hidden.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"
#include "src/common.hpp"
#include <format>
#include <string_view>
#include <vector>

using namespace ftxui;

auto interface_check_hidden(const Component root, disk_t &disk,
                                    const int hpa_dco) -> int
{
  std::vector<std::string_view> entries = {
      "[ Continue ] Continue even if there is hidden data",
      "[ Quit ]",
  };
  int selected = 0;

  MenuOption option;
  auto screen     = App::Fullscreen();
  option.on_enter = screen.ExitLoopClosure();
  const auto menu = Menu(&entries, &selected, option);
  auto dialog     = Renderer(menu, [&]() {
    return vbox({
               text(disk.description_short(disk)),
               separator(),
               text("Hidden sectors are present.") | color(Color::Yellow),
               (disk.sector_size != 0)
                   ? text(std::format("size       {} sectors",
                                      (disk.disk_real_size / disk.sector_size)))
                   : emptyElement(),
               (disk.user_max != 0)
                   ? text(std::format("user_max   {} sectors", disk.user_max))
                   : emptyElement(),
               (disk.native_max != 0)
                   ? text(std::format("native_max {} sectors", disk.native_max+1))
                   : emptyElement(),
               (disk.dco != 0)
                   ? text(std::format("dco        {} sectors", disk.dco+1))
                   : emptyElement(),
               (hpa_dco & 1)
                   ? text("Host Protected Area (HPA) present.")
                   : emptyElement(),
               (hpa_dco & 2)
                   ? text("Device Configuration Overlay (DCO) present.")
                   : emptyElement(),
               separatorEmpty(),
               menu->Render(),
           }) |
           size(WIDTH, GREATER_THAN, 30) | border | center;
  });
  bool show_modal = true;
  screen.Loop(root | Modal(dialog, &show_modal));

  return selected;
}
