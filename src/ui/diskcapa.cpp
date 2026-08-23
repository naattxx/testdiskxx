#include "diskcapa.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"
#include "src/common.hpp"
#include "src/log.hpp"
#include <string_view>
#include <vector>

using namespace ftxui;

auto interface_check_disk_capacity(const Component root, disk_t &disk_car)
    -> int
{
  /* Test for LBA28 limitation */
  if (disk_car.geom.sectors_per_head > 0 &&
      disk_car.geom.cylinders ==
          (((1 << 28) - 1) / disk_car.geom.heads_per_cylinder /
           disk_car.geom.sectors_per_head))
  {
    log_warning("LBA28 limitation\n");

    std::vector<std::string_view> entries = {
        "[ Quit ] The HD is bigger, it's safer to enable LBA48 support first.",
        "[ Continue ] The HD is really 137 GB only.",
    };
    int selected = 0;

    MenuOption option;
    auto screen     = App::Fullscreen();
    option.on_enter = screen.ExitLoopClosure();
    const auto menu = Menu(&entries, &selected, option);
    auto dialog     = Renderer(menu, [&]() {
      return vbox({
                 text(disk_car.description(disk_car)),
                 separator(),
                 paragraph(
                     "The Hard disk size seems to be 137GB.\n"
                     "Support for 48-bit Logical Block "
                     "Addressing (LBA) is needed to access"
                     "hard disks larger than 137 GB."
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(_WIN32)
                     "Update Windows to support LBA48 (minimum: W2K "
                     "SP4 or XP SP1)"
#endif
                 ) | color(Color::Yellow),
                 separatorEmpty(),
                 menu->Render(),
             }) |
             size(WIDTH, GREATER_THAN, 30) | border | center;
    });
    bool show_modal = true;
    screen.Loop(root | Modal(dialog, &show_modal));

    if (selected == 0)
      return 1;
  }
  return 0;
}
