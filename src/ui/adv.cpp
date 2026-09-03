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
#include "src/adv.hpp"
#include "src/common.hpp"
#include "src/guid_cmp.hpp"
#include "src/intrf.hpp"
#include "src/log.hpp"
#include "src/log_part.hpp"
#include "src/part/fat.hpp"
#include "src/part/ntfs.hpp"
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

static auto is_part_hfs(const partition_t &partition) -> int
{
  if (partition.part_type_i386 == P_HFS || partition.part_type_mac == PMAC_HFS)
    return 1;
  if (partition.part_type_gpt == GPT_ENT_TYPE_MAC_HFS)
    return 1;
  return 0;
}

static auto is_part_hfsp(const partition_t &partition) -> int
{
  if (partition.part_type_i386 == P_HFSP || partition.part_type_mac == PMAC_HFS)
    return 1;
  if (partition.part_type_gpt == GPT_ENT_TYPE_MAC_HFS)
    return 1;
  return 0;
}

static auto is_exfat(const partition_t &partition) -> int
{
  return (is_part_ntfs(partition) || partition.upart_type == UP_EXFAT);
}

static auto is_hfs(const partition_t &partition) -> int
{
  return (is_part_hfs(partition) || partition.upart_type == UP_HFS);
}

static auto is_hfsp(const partition_t &partition) -> int
{
  return (is_part_hfsp(partition) || partition.upart_type == UP_HFSP ||
          partition.upart_type == UP_HFSX);
}

static auto is_linux(const partition_t &partition) -> int
{
  if (is_part_linux(partition))
    return 1;
  switch (partition.upart_type)
  {
  case UP_CRAMFS:
  case UP_EXT2:
  case UP_EXT3:
  case UP_EXT4:
  case UP_JFS:
  case UP_RFS:
  case UP_RFS2:
  case UP_RFS3:
  case UP_RFS4:
  case UP_XFS:
  case UP_XFS2:
  case UP_XFS3:
  case UP_XFS4:
  case UP_XFS5:
    return 1;
  default:
    break;
  }
  return 0;
}

static auto adv_get_boot_description(const partition_t &partition)
    -> std::string_view
{
  if (is_part_linux(partition))
  {
    return "Locate ext2/ext3/ext4 backup superblock";
  }
  if (is_part_hfs(partition) || is_part_hfsp(partition))
  {
    return "Locate HFS/HFS+ backup volume header";
  }
  if (is_linux(partition))
  {
    return "Locate ext2/ext3/ext4 backup superblock";
  }
  if (is_hfs(partition) || is_hfsp(partition))
  {
    return "Locate HFS/HFS+ backup volume header";
  }
  return "Boot sector recovery";
}

static void adv_get_options_for_partition(const partition_t &partition,
                                          bool &hasBoot, bool &hasSuperblock,
                                          bool &hasList, bool &hasUndelete)
{
  if (is_part_fat(partition))
  {
    hasBoot = hasUndelete = true;
    hasSuperblock = hasList = false;
  }
  else if (is_part_ntfs(partition))
  {
    hasBoot = hasList = hasUndelete = true;
    hasSuperblock                   = false;
  }
  else if (is_part_linux(partition))
  {
    hasBoot       = false;
    hasSuperblock = true;
    hasUndelete   = partition.upart_type == UP_EXT2;
    hasList       = !hasUndelete;
  }
  else if (is_part_hfs(partition) || is_part_hfsp(partition))
  {
    hasBoot = hasList = hasUndelete = false;
    hasSuperblock                   = true;
  }
  else if (is_fat(partition))
  {
    hasBoot = hasUndelete = true;
    hasSuperblock = hasList = false;
  }
  else if (is_ntfs(partition) || is_exfat(partition))
  {
    hasBoot = hasList = hasUndelete = true;
    hasSuperblock                   = false;
  }
  else if (is_linux(partition))
  {
    hasList = hasSuperblock = true;
    hasBoot                 = false;
    hasUndelete             = partition.upart_type == UP_EXT2;
  }
  else if (is_hfs(partition) || is_hfsp(partition))
  {
    hasBoot = hasList = hasUndelete = false;
    hasSuperblock                   = true;
  }
  else
    hasBoot = hasList = hasUndelete = hasSuperblock = false;
}

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

  bool hasBoot, hasSuperblock, hasList, hasUndelete;
  adv_get_options_for_partition(list_part.front(), hasBoot, hasSuperblock,
                                hasList, hasUndelete);
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
            adv_get_options_for_partition(list_part[selected_part], hasBoot,
                                          hasSuperblock, hasList, hasUndelete);
          },
          buttonOptions
      ),
      Maybe(Button("[  Boot  ]", []() -> void {},
                   {
                       .transform = [&](EntryState s) -> Element {
                         if (s.focused)
                         {
                           buttonDescription = text(adv_get_boot_description(
                               list_part[selected_part]
                           ));
                           return hbox({
                               text(">" + s.label) | inverted | bold,
                               text(" "),
                           });
                         }

                         return text(" " + s.label + " ");
                       },
                   }),
            &hasBoot),
      Maybe(Button(
                "[Superblock]", []() -> void {}, buttonOptions
            ),
            &hasSuperblock),
      Maybe(Button(
                "[  List  ]"
                "List and copy files",
                []() -> void {}, buttonOptions
            ),
            &hasList),
      Maybe(Button(
                "[Undelete]"
                "File undelete",
                []() -> void {}, buttonOptions
            ),
            &hasUndelete),
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
          adv_get_options_for_partition(list_part[selected_part], hasBoot,
                                        hasSuperblock, hasList, hasUndelete);
          return true;
        }
        if (e == Event::ArrowDown && selected_part < list_part.size() - 1)
        {
          selected_part++;
          adv_get_options_for_partition(list_part[selected_part], hasBoot,
                                        hasSuperblock, hasList, hasUndelete);
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
