#include "ntfs_udl.hpp"
#include "config.h"
#include "src/common.hpp"
#include "src/dir_common.hpp"

#if defined(HAVE_LIBNTFS) || defined(HAVE_LIBNTFS3G)
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/node.hpp"
#include "ftxui/screen/color.hpp"
#include "ftxui/screen/terminal.hpp"
#include "src/dir.hpp"
#include "src/intrf.hpp"
#include "src/log.hpp"
#include "src/part/ntfs_inc.hpp"
#include "src/part/ntfs_udl.hpp"
#include "src/ui/askloc.hpp"
#include "src/ui/intrfn.hpp"
#include <algorithm>
#include <cstdint>
#include <format>
#include <future>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace ftxui;


static void undelete(const Component &root, ntfs_dir_struct *dir_data,
                     uint64_t inode)
{
  std::future<int> future_res =
      std::async(undelete_file, dir_data->vol, inode, dir_data->local_dir);

  auto wait_screen                 = App::Fullscreen();
  static constexpr bool show_modal = true;
  wait_screen.Loop(root | Modal(Renderer([&] -> Element {
                                  using namespace std::chrono_literals;
                                  if (future_res.wait_for(10ms) ==
                                      std::future_status::ready)
                                    wait_screen.Exit();
                                  wait_screen.RequestAnimationFrame();
                                  return paragraph("Copying, please wait...") |
                                         size(WIDTH, ftxui::LESS_THAN,
                                              Terminal::Size().dimx * 0.75f) |
                                         borderHeavy | center;
                                }),
                                &show_modal));

  const int res = future_res.get();

  if (res < -1)
    display_message(root, "Copy failed!");
  else if (res == -1)
    display_message(root, "Copy done! (Failed to copy some files)");
  else
    display_message(root, "Copy done!");
}

static void undelete_selected(const Component &root,
                              ntfs_dir_struct *ntfs_dir_data,
                              dir_list_t &dir_list)
{
  unsigned int file_ok         = 0;
  unsigned int file_bad        = 0;
  std::future<void> future_res = std::async([&] -> void {
    for (auto &file :
         dir_list | std::views::filter([](const auto &file) -> bool {
           return file.status.marked;
         }))
    {
      if (undelete_file(ntfs_dir_data->vol, file.st_ino,
                        ntfs_dir_data->local_dir) < 0)
        file_bad++;
      else
      {
        file.status.marked = false;
        file_ok++;
      }
    }
  });

  auto wait_screen                 = App::Fullscreen();
  static constexpr bool show_modal = true;
  wait_screen.Loop(
      root | Modal(Renderer([&] -> Element {
                     using namespace std::chrono_literals;
                     if (future_res.wait_for(10ms) == std::future_status::ready)
                       wait_screen.Exit();
                     wait_screen.RequestAnimationFrame();
                     return paragraph(std::format(
                                "Copying, please wait... {} files done", file_ok
                            )) |
                            size(WIDTH, ftxui::LESS_THAN,
                                 Terminal::Size().dimx * 0.75f) |
                            borderHeavy | center;
                   }),
                   &show_modal)
  );

  if (file_ok == 0)
  {
    display_message(root, "Copy failed!");
  }
  else
  {
    display_message(root, std::format("Copy done! ({}/{})", file_ok,
                                      (file_ok + file_bad)));
  }
}

#else
#include <utility>
#endif

// dir_data needs to be a ntfs_dir_struct
void ntfs_undelete_menu(disk_t &disk, const partition_t &partition,
                        dir_data_t *&ntfs_dir_data, dir_list_t &dir_list)
{
#if defined(HAVE_LIBNTFS) || defined(HAVE_LIBNTFS3G)
  log_info("\n");
  log_list_file(disk, partition, *ntfs_dir_data, dir_list);
  auto screen = App::Fullscreen();

  int currentFile = 0;
  std::vector<std::string> menuEntries {
      std::from_range, dir_list | std::views::transform(&dir_aff_entry)};

  std::string_view error_msg;

  if (!error_msg.empty())
    log_warning(error_msg);

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
                       }, },
  }) |
      CatchEvent([&](const Event &event) -> bool {
        if (event == Event::q)
        {
          screen.Exit();
          return true;
        }
        if (event == Event::Character(':'))
        {
          dir_list[currentFile].status.marked =
              !dir_list[currentFile].status.marked;
          return true;
        }
        if (event == Event::a)
        {
          std::ranges::for_each(
              dir_list,
              [marked_status = !dir_list[currentFile].status.marked](auto &file)
                  -> void { file.status.marked = marked_status; }
          );
          return true;
        }
        if (event == Event::c)
        {
          if (LINUX_S_ISDIR(dir_list[currentFile].st_mode) != 0)
            ask_location(
                ntfs_dir_data->local_dir,
                std::format("Please select a destination where '{}' and "
                            "any files below will be copied.",
                            dir_list[currentFile].name)
            );
          else if (LINUX_S_ISREG(dir_list[currentFile].st_mode) != 0)
            ask_location(
                ntfs_dir_data->local_dir,
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
          if (!ntfs_dir_data->local_dir.empty())
          {
            undelete(root, static_cast<ntfs_dir_struct *>(ntfs_dir_data),
                     dir_list[currentFile].st_ino);
          }
          return true;
        }
        if (event == Event::C)
        {
          ask_location(ntfs_dir_data->local_dir,
                       "Please select a destination where the marked files "
                       "will be copied.");
          if (!ntfs_dir_data->local_dir.empty())
          {
            undelete_selected(root,
                              static_cast<ntfs_dir_struct *>(ntfs_dir_data),
                              dir_list);
          }
          return true;
        }
        if (event == Event::f)
        {
          // TODO:
          return true;
        }
        if (event == Event::r)
        {
          // TODO:
          return true;
        }
        if (event == Event::s)
        {
          // TODO:
          return true;
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
        dir_list.empty()
            ? text("No deleted file found.")
            : vbox({
                  text("Deleted files"),
                  separatorEmpty(),
                  dir_menu->Render() | vscroll_indicator | yframe | yflex,
                  separatorEmpty(),
                  hbox({
                      text("Use "),
                      bold(text("':'")),
                      dir_list[currentFile].status.marked
                          ? text(" to deselect the current file, ")
                          : text(" to select the current file, "),
                      bold(text("'a'")),
                      dir_list[currentFile].status.marked
                          ? text(" to deselect all files")
                          : text(" to select all files"),
                  }),
                  hbox({
                      bold(text("'C'")),
                      text(" to copy the selected files, "),
                      bold(text("'c'")),
                      text(" to copy the current file, "),
                      bold(text("'q'")),
                      text(" to quit"),
                  }),
              }),
    });
  });
  screen.Loop(root);
#else
  std::unreachable();
#endif
}
