/*

    File: ext2_sb.c

    Copyright (C) 2008 Christophe GRENIER <grenier@cgsecurity.org>

    This software is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/node.hpp"
#include "ftxui/dom/table.hpp"
#include "src/common.hpp"
#include "src/guid_cmp.hpp"
#include "src/intrf.hpp"
#include "src/log.hpp"
#include <config.h>
#include <format>
#include <string>
#include <vector>

using namespace ftxui;

auto interface_superblock(const Component root, disk_t &disk,
                          const list_part_t &list_part) -> int
{
  auto screen = App::Fullscreen();

  auto quitButton =
      Button("Quit", screen.ExitLoopClosure(), ButtonOption::Ascii());

  std::vector<std::vector<std::string>> rows{
      {"Partition", "Start", "End", "Size in sectors"}
  };

  for (const partition_t *old_part = nullptr;
       const partition_t &partition : list_part)
  {
    if (old_part == nullptr || old_part->part_offset != partition.part_offset ||
        old_part->part_size != partition.part_size ||
        old_part->part_type_gpt != partition.part_type_gpt ||
        old_part->part_type_i386 != partition.part_type_i386 ||
        old_part->part_type_sun != partition.part_type_sun ||
        old_part->part_type_mac != partition.part_type_mac ||
        old_part->upart_type != partition.upart_type)
    {
      rows.push_back(aff_part_aux(AFF_PART_BASE, disk, partition));
      old_part = &partition;
    }
    if (partition.blocksize != 0)
      rows.push_back({
          std::format("superblock {}, blocksize={} [{}]",
                      partition.sb_offset / partition.blocksize,
                      partition.blocksize, partition.fsname),
      });
  }

  Table table(rows);
  table.SelectColumns(1, 2).Decorate(align_right);
  table.SelectRow(0).Decorate(center);
  table.SelectRow(0).Decorate(bold);
  table.SelectRow(0).Separator(EMPTY);

  Element tableElement = table.Render();

  Render(screen, tableElement);
  log_info(screen.ToString());

  auto dialog = Renderer(quitButton, [&]() -> Element {
    return vbox({
               text(disk.description(disk)),
               separator(),
               tableElement,
               separator(),
               (!list_part.empty())
                   ? vflow({
                         text("To repair the filesystem using alternate "
                              "superblock, run "),
                         bold(text(std::format(
                             "fsck.ext{} -p -b superblock -B blocksize device",
                             (list_part.front().upart_type == UP_EXT2
                                  ? 2
                                  : (list_part.front().upart_type == UP_EXT3
                                         ? 3
                                         : 4))
                         ))),
                     })
                   : emptyElement(),
               separatorEmpty(),
               quitButton->Render(),
               text("Return to Advanced menu") | hcenter,
           }) |
           borderHeavy | center;
  });

  bool show_modal = true;
  screen.Loop(root | Modal(dialog, &show_modal));

  return 0;
}
