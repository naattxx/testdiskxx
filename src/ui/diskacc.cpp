#include "diskacc.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"
#include "src/common.hpp"
#include "src/log.hpp"
#include "src/utils.hpp"
#include <string_view>
#include <vector>

using namespace ftxui;

auto interface_check_disk_access(const Component root, disk_t &disk_car) -> int
{
  if ((disk_car.access_mode & TESTDISK_O_RDWR) == TESTDISK_O_RDWR)
    return 0;
  log_warning("Media is opened in read-only.\n");

  std::vector<std::string_view> entries = {
      "[ Continue ] Continue even if write access isn't available",
      "[ Quit ] Return to disk selection",
  };
  int selected = 0;

  MenuOption option;
  auto screen     = App::Fullscreen();
  option.on_enter = screen.ExitLoopClosure();
  const auto menu = Menu(&entries, &selected, option);
  auto dialog     = Renderer(menu, [&]() -> Element {
    return vbox({
               text(disk_car.description_short(disk_car)),
               separator(),
               paragraph("Write access for this media is not available.\n"
                         "TestDisk won't be able to modify it.\n") |
                   color(Color::Yellow),
               (!isAdmin())
                   ? paragraph(
#ifdef DJGPP
#elif defined(__CYGWIN__) || defined(__MINGW32__) || defined(_WIN32)
                   "- You may need to be administrator to have write access.\n"
                   "select TestDisk, right-click and choose \"Run as administrator\".\n"
#elif __has_include(<unistd.h>)
                   "- You may need to be root to have write access.\n"
                   "Use the sudo command to launch TestDisk.\n"
                   "- Check the OS permissions for this file or device."
#endif
#if defined(__APPLE__)
                         "\n- partitions from this disk must not be mounted:\n"
                         "Open the Disk Utility (In Finder -> Application -> "
                         "Utility folder)\n"
                         "and press Unmount button for each volume from this "
                         "disk"
#endif
                     )
                   : emptyElement(),
               paragraph("- This media may be physically write-protected, "
                         "check the jumpers.\n"),
               separatorEmpty(),
               menu->Render(),
           }) |
           size(WIDTH, GREATER_THAN, 30) | border | center;
  });
  bool show_modal = true;
  screen.Loop(root | Modal(dialog, &show_modal));

  return selected;
}
