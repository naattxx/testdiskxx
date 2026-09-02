#include "adv.hpp"
#include "chgtype.hpp"
#include "config.h"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/table.hpp"
#include "src/common.hpp"
#include "src/intrf.hpp"
#include "src/log.hpp"
#include "src/log_part.hpp"
#include "src/ui/intrfn.hpp"
#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace ftxui;

extern const arch_fnct_t arch_gpt;
extern const arch_fnct_t arch_i386;
extern const arch_fnct_t arch_mac;
extern const arch_fnct_t arch_none;
extern const arch_fnct_t arch_sun;
extern const arch_fnct_t arch_xbox;

void interface_adv(disk_t &disk, const int verbose, const bool dump,
                   const bool expert)
{
  log_info("\nInterface Advanced\n");
  list_part_t list_part = disk.arch->read_part(disk, verbose, 0);
  log_all_partitions(disk, list_part);

  auto screen = App::Fullscreen();
  Component root;

  auto buttonDescription = text("");
  auto buttonOptions     = ButtonOption();
  buttonOptions.transform =
      [&buttonDescription](const EntryState &s) -> Element {
    if (s.focused)
    {
      buttonDescription = text(s.label.substr(s.label.find(']') + 1));
      return hbox({
          text(">" + s.label.substr(0, s.label.find(']') + 1)) | inverted |
              bold,
          text(" "),
      });
    }

    return text(" " + s.label.substr(0, s.label.find(']') + 1) + " ");
  };

  int selected_part{0};
  std::vector<std::vector<std::string>> rows{
      {"", "", "Partition", "Start", "End", "Size in sectors", "", ""}
  };

  Components buttons{
      Button(
          "[  Type  ]"
          "Change type, this setting will not be saved on disk",
          [&]() -> void {
            change_part_type_interface(root, disk, list_part[selected_part]);

            // refresh row
            rows[selected_part + 1] =
                aff_part_aux(AFF_PART_ORDER | AFF_PART_STATUS, disk,
                             list_part[selected_part]);
          },
          buttonOptions
      ),
      Button(
          "[  Boot  ]"
          "Boot sector recovery",
          []() -> void {}, buttonOptions
      ),
      Button(
          "[  List  ]"
          "List and copy files",
          []() -> void {}, buttonOptions
      ),
      Button(
          "[Undelete]"
          "File undelete",
          []() -> void {}, buttonOptions
      ),
      Button(
          "[Image Creation]"
          "Create an image",
          []() -> void {}, buttonOptions
      ),
      Button("[  Quit  ]"
             "Return to main menu",
             screen.ExitLoopClosure(), buttonOptions),
  };
  if (expert)
    buttons.insert(buttons.end() - 1,
                   Button(
                       "[  Add   ]"
                       "Add temporary partition (Expert only)",
                       []() -> void {}, buttonOptions
                   ));
  auto buttonsContainer =
      Container::Horizontal(buttons) | CatchEvent([&](Event e) -> bool {
        if (e == Event::ArrowUp && selected_part > 0)
        {
          selected_part--;
          return true;
        }
        if (e == Event::ArrowDown && selected_part < list_part.size() - 1)
        {
          selected_part++;
          return true;
        }
        return false;
      });

  for (const auto &partition : list_part)
  {
    rows.push_back(aff_part_aux(AFF_PART_ORDER | AFF_PART_STATUS, disk,
                                partition));
  }

  root = Renderer(buttonsContainer, [&]() -> Element {
    Table table(rows);
    table.SelectColumns(3, 6).Decorate(align_right);
    table.SelectRow(0).Decorate(bold);
    table.SelectRow(0).Separator(EMPTY);
    table.SelectRow(selected_part + 1).Decorate(inverted);

    return vbox({
        hflow({text("TestDisk++ "), bold(text(VERSION)),
               text(", Data Recovery Utility, "), text(TESTDISKDATE)}),
        text("naattxx"),
        text("https://github.com/naattxx/testdiskxx"),
        separatorEmpty(),
        text(disk.description(disk)),
        vbox({
            table.Render() | yframe | vscroll_indicator,
            filler(),
            (disk.arch->msg_part_type != nullptr)
                ? text(disk.arch->msg_part_type)
                : emptyElement(),
        }) | xframe |
            border | yflex,
        buttonsContainer->Render(),
        buttonDescription | hcenter,
    });
  });

  if (list_part.empty())
    display_message(root, "No partition available.");
  else
    screen.Loop(root);
}
