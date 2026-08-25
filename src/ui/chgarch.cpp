#include "chgarch.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"
#include "src/common.hpp"
#include "src/log.hpp"
#include <array>
#include <string>
#include <string_view>
#include <vector>

using namespace ftxui;

extern const arch_fnct_t arch_none;
extern const arch_fnct_t arch_i386;
extern const arch_fnct_t arch_gpt;
extern const arch_fnct_t arch_humax;
extern const arch_fnct_t arch_mac;
extern const arch_fnct_t arch_sun;
extern const arch_fnct_t arch_xbox;

auto change_arch_type(const Component root, disk_t &disk, const int verbose)
    -> int
{
  // arch_list must match the order from entries
  const std::array<const arch_fnct_t *, 7> arch_list{
      &arch_i386, &arch_gpt, &arch_humax, &arch_mac,
      &arch_none, &arch_sun, &arch_xbox};

  int selected;
  for (selected = 0; static_cast<unsigned>(selected) < arch_list.size() &&
                     disk.arch != arch_list[selected];
       selected++)
    ;
  if (selected == arch_list.size())
  {
    selected  = 0;
    disk.arch = arch_list[selected];
  }

  // interface
  {
    using namespace std::literals;
    std::vector<std::string> entries = {
        ("["s + arch_i386.part_name + "  ] Intel/PC partition"),
        ("["s + arch_gpt.part_name +
         "] EFI GPT partition map (Mac i386, some x86_64...)"),
        ("["s + arch_humax.part_name + "  ] Humax partition table"),
        ("["s + arch_mac.part_name + "    ] Apple partition map (legacy)"),
        ("["s + arch_none.part_name + "   ] Non partitioned media"),
        ("["s + arch_sun.part_name + "    ] Sun Solaris partition"),
        ("["s + arch_xbox.part_name + "   ] XBox partition"),
        "[Return ] Return to disk selection",
    };

    MenuOption option;
    auto screen     = App::Fullscreen();
    option.on_enter = screen.ExitLoopClosure();
    const auto menu = Menu(&entries, &selected, option);
    auto dialog     = Renderer(menu, [&]() -> Element {
      return vbox({
                 text(disk.description_short(disk)),
                 separator(),
                 text("Please select the partition table type, press Enter "
                      "when done."),
                 menu->Render(),
                 separatorEmpty(),
                 (disk.arch_autodetected)
                     ? hflow({text("Hint: "),
                              text(disk.arch_autodetected->part_name) |
                                  color(Color::Green),
                              text(" partition table type has been detected.")})
                     : emptyElement(),
                 (disk.arch_autodetected != &arch_none)
                     ? paragraph("Note: Do NOT select 'None' for media with "
                                 "only a single partition. It's very "
                                 "rare for a disk to be 'Non-partitioned'.")
                     : emptyElement(),
             }) |
             size(WIDTH, GREATER_THAN, 30) | border | center;
    });
    bool show_modal = true;
    screen.Loop(root | Modal(dialog, &show_modal));

    if (static_cast<unsigned>(selected) ==
        arch_list.size()) // == entries.size() -1
      return 1;
  }
  disk.autoset_unit();
  disk.update_geometry(verbose);
  log_info(disk.description_short(disk));
  log_info("Partition table type: {}", disk.arch->part_name);
  return 0;
}
