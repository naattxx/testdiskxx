/*

    File: dirpart.c

    Copyright (C) 1998-2008 Christophe GRENIER <grenier@cgsecurity.org>

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
#include "dirpart.hpp"
#include "askloc.hpp"
#include "config.h"
#include "dir.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"
#include "intrfn.hpp"
#include "src/adv.hpp"
#include "src/common.hpp"
#include "src/dir.hpp"
#include "src/dir_common.hpp"
#include "src/intrf.hpp"
#include "src/log.hpp"
#include "src/log_part.hpp"
#include "src/part/exfat_dir.hpp"
#include "src/part/ext2_dir.hpp"
#include "src/part/fat.hpp"
#include "src/part/fat_dir.hpp"
#include "src/part/ntfs.hpp"
#include "src/part/ntfs_dir.hpp"
#include "src/part/rfs_dir.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace ftxui;

static auto dir_aff_entry(const file_info_t &file_info) -> std::string
{
  std::string datestr;
  set_datestr(datestr, file_info.td_mtime);
  const std::string str = mode_string(file_info.st_mode);
  return std::format("{} {:5} {:5} {:9} {} {}", str, file_info.st_uid,
                     file_info.st_gid, file_info.st_size, datestr,
                     file_info.name);
}

static auto dir_partition_init(disk_t &disk, const partition_t &partition,
                               const int verbose, const int expert)
    -> std::expected<dir_data_t *, dir_partition_t>
{
  std::expected<dir_data_t *, dir_partition_t> res;
  if (is_part_fat(partition))
  {
    if ((res = dir_partition_fat_init(disk, partition, verbose)))
      return res;
  }
  else if (is_part_ntfs(partition))
  {
    if ((res = dir_partition_ntfs_init(disk, partition, verbose, expert)))
      return res;
    if ((res = dir_partition_exfat_init(disk, partition, verbose)))
      return res;
  }
  else if (is_part_linux(partition))
  {
    if ((res = dir_partition_ext2_init(disk, partition, verbose)))
      return res;
    if ((res = dir_partition_reiser_init(disk, partition, verbose)))
      return res;
  }
  switch (partition.upart_type)
  {
  case UP_FAT12:
  case UP_FAT16:
  case UP_FAT32:
    return dir_partition_fat_init(disk, partition, verbose);
  case UP_EXT4:
  case UP_EXT3:
  case UP_EXT2:
    return dir_partition_ext2_init(disk, partition, verbose);
  case UP_RFS:
  case UP_RFS2:
  case UP_RFS3:
    return dir_partition_reiser_init(disk, partition, verbose);
  case UP_NTFS:
    return dir_partition_ntfs_init(disk, partition, verbose, expert);
  case UP_EXFAT:
    return dir_partition_exfat_init(disk, partition, verbose);
  default:
    return std::unexpected(DIR_PART_ENOIMP);
  }
}

auto dir_partition(disk_t &disk, const partition_t &partition,
                   const int verbose, const bool expert) -> dir_partition_t
{
  log_info("\n");
  auto screen = App::Fullscreen();

  auto tmp = dir_partition_init(disk, partition, verbose, expert);

  dir_data_t *dir_data = tmp.value_or(nullptr);
  dir_list_t dir_list{};

  static constexpr int first_file_index = 2;
  int currentFile                       = 0;
  std::vector<std::string> menuEntries;

  std::string_view error_msg;
  dir_partition_t res = tmp.error_or(DIR_PART_OK);
  switch (res)
  {
  case DIR_PART_OK:
    if (verbose > 0)
    {
      log_info("dir_partition inode={}", dir_data->current_inode);
      log_partition(disk, partition);
    }

    dir_data->get_dir(disk, partition, dir_data->current_inode, dir_list);
    dir_aff_log(*dir_data, dir_list);

    menuEntries = dir_list | std::views::transform(&dir_aff_entry) |
                  std::ranges::to<std::vector<std::string>>();

    error_msg = "";
    break;
  case DIR_PART_ENOIMP:
    log_partition(disk, partition);
    error_msg = "Support for this filesystem hasn't been implemented.";
    break;
  case DIR_PART_ENOSYS:
    log_partition(disk, partition);
    error_msg =
        "Support for this filesystem wasn't enabled during compilation.";
    break;
  case DIR_PART_EIO:
    log_partition(disk, partition);
    error_msg = "Can't open filesystem. Filesystem seems damaged.";
    break;
  }
  if (!error_msg.empty())
    log_warning(error_msg);

  auto enter_folder = [&] -> void {
    auto new_path =
        std::filesystem::weakly_canonical(dir_data->current_directory /
                                          dir_list[currentFile].name);
    if (LINUX_S_ISDIR(dir_list[currentFile].st_mode) &&
        new_path != dir_data->current_directory)
    {
      dir_data->current_directory = std::move(new_path);
      dir_data->current_inode     = dir_list[currentFile].st_ino;
      dir_list.clear();
      dir_data->get_dir(disk, partition, dir_data->current_inode, dir_list);
      menuEntries = dir_list | std::views::transform(&dir_aff_entry) |
                    std::ranges::to<std::vector<std::string>>();
    }
  };

  Component root;
  auto dir_menu =
      Menu(&menuEntries, &currentFile,
           {
               .entries_option =
                  {
                      .transform = [&](const EntryState &state) -> Element {
                        const bool marked =
                            dir_list[state.index].status.marked;

                        Element e =
                            text(std::string(state.active ? ">" : " ") +
                                (marked ? "*" : " ") + state.label);
                        if (marked)
                          e |= color(Color::Green);

                        if (state.focused)
                          e |= inverted;
                        if (state.active)
                          e |= bold;

                        return e;
                      },
                  },
               .on_enter = enter_folder,
           }) |
      CatchEvent([&](const Event &event) -> bool {
        if (event == Event::q)
        {
          screen.Exit();
          return true;
        }
        if (event == Event::h)
        {
          if ((dir_data->capabilities & CAPA_LIST::DELETED) != CAPA_LIST::NONE)
            dir_data->param ^= FLAG_LIST::DELETED;
          else if ((dir_data->capabilities & CAPA_LIST::ADS) != CAPA_LIST::NONE)
            dir_data->param ^= FLAG_LIST::ADS;

          dir_list.clear();
          dir_data->get_dir(disk, partition, dir_data->current_inode, dir_list);

          return true;
        }
        if (event == Event::ArrowRight)
        {
          enter_folder();
          return true;
        }
        if (event == Event::ArrowLeft)
        {
          currentFile = 1; // ".."
          enter_folder();
          return true;
        }
        if (event == Event::Character(':'))
        {
          dir_list[std::max(currentFile, first_file_index)].status.marked =
              !dir_list[std::max(currentFile, first_file_index)].status.marked;
        }
        if (event == Event::a)
        {
          std::for_each(dir_list.begin() + first_file_index, dir_list.end(),
                        [marked_status =
                             !dir_list[std::max(currentFile, first_file_index)]
                                  .status.marked](auto &file) -> void {
                          file.status.marked = marked_status;
                        });
        }
        if (event == Event::c && currentFile >= first_file_index)
        {
          if (LINUX_S_ISDIR(dir_list[currentFile].st_mode) != 0)
            ask_location(
                dir_data->local_dir,
                std::format("Please select a destination where '{}' and "
                            "any files below will be copied.",
                            dir_list[currentFile].name)
            );
          else if (LINUX_S_ISREG(dir_list[currentFile].st_mode) != 0)
            ask_location(
                dir_data->local_dir,
                std::format(
                    "Please select a destination where '{}' will be copied.",
                    dir_list[currentFile].name
                )
            );
          else
          {
            display_message(root, std::format("'{}' is not a regular file",
                                              dir_list[currentFile].name));
            return true;
          }
          if (!dir_data->local_dir.empty())
          {
            unsigned int copy_ok {0}, copy_bad {0};
            if (LINUX_S_ISDIR(dir_list[currentFile].st_mode) != 0)
              copy_dir(root, disk, partition, dir_data, dir_list[currentFile],
                       copy_ok, copy_bad);
            else // if (LINUX_S_ISREG(dir_list[currentFile].st_mode) != 0)
              copy_file(root, disk, partition, dir_data, dir_list[currentFile],
                        copy_ok, copy_bad);
          }
          return true;
        }
        if (event == Event::C)
        {
          ask_location(dir_data->local_dir,
                       "Please select a destination where the marked files "
                       "will be copied.");
          if (!dir_data->local_dir.empty())
          {
            unsigned int copy_ok {0}, copy_bad {0};
            copy_selection(root, disk, partition, dir_data, dir_list, copy_ok,
                           copy_bad);
          }
        }
        return false;
      });

  root = Renderer(dir_menu, [&] -> Element {
    return vbox({
        hflow({
            text("TestDisk++ "),
            bold(text(VERSION)),
            text(", Data Recovery Utility, "),
            text(TESTDISKDATE),
        }),
        separatorEmpty(),
        text(std::format("{:n:>4}",
                         aff_part_aux(AFF_PART::ORDER | AFF_PART::STATUS, disk,
                                      partition))),
        dir_data
            ? vbox({
                  hflow({
                      text("Directory "),
                      text(dir_data->current_directory.string()),
                  }),
                  separatorEmpty(),
                  dir_menu->Render() | vscroll_indicator | yframe | yflex,
                  separatorEmpty(),
                  hbox({
                      text("Use "),
                      dir_data->current_directory !=
                              dir_data->current_directory.root_directory()
                          ? hbox({
                                bold(text("Left")),
                                text(" arrow to go back, "),
                            })
                          : emptyElement(),
                      hbox({
                          bold(text("Right")),
                          text(" to change directory, "),
                      }),
                      (dir_data->capabilities & CAPA_LIST::DELETED) !=
                              CAPA_LIST::NONE
                          ? hbox({
                                bold(text("'h'")),
                                (dir_data->param & FLAG_LIST::DELETED) == 0
                                    ? text(" to unhide deleted files")
                                    : text(" to hide deleted files"),
                            })
                          : emptyElement(),
                      (dir_data->capabilities & CAPA_LIST::ADS) !=
                              CAPA_LIST::NONE
                          ? hbox({
                                bold(text("'h'")),
                                (dir_data->param & FLAG_LIST::ADS) == 0
                                    ? text(" to unhide Alternate Data Stream")
                                    : text(" to hide Alternate Data Stream"),
                            })
                          : emptyElement(),
                  }),
                  hbox({
                      bold(text("'q'")),
                      text(" to quit"),
                      dir_list.size() > first_file_index
                          ? hflow({
                                text(", "),
                                bold(text("':'")),
                                dir_list[std::max(currentFile,
                                                  first_file_index)]
                                        .status.marked
                                    ? text(" to deselect the current file, ")
                                    : text(" to select the current file, "),
                                bold(text("'a'")),
                                dir_list[std::max(currentFile,
                                                  first_file_index)]
                                        .status.marked
                                    ? text(" to deselect all files")
                                    : text(" to select all files"),
                            })
                          : emptyElement(),
                  }),
                  dir_list.size() > first_file_index
                      ? hbox({
                            bold(text("'C'")),
                            text(" to copy the selected files"),
                            currentFile >= first_file_index
                                ? hflow({
                                      text(", "),
                                      bold(text("'c'")),
                                      text(" to copy the current file"),
                                  })
                                : emptyElement(),
                        })
                      : emptyElement(),
              })
            : text(error_msg),
    });
  });
  screen.Loop(root);

  if (dir_data)
  {
    dir_data->close();
    delete dir_data;
  }
  return res;
}
