/*

    File: askloc.c

    Copyright (C) 1998-2009 Christophe GRENIER <grenier@cgsecurity.org>

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

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"
#include "src/dir_common.hpp"
#include "src/ui/intrfn.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <format>
#include <future>
#include <ranges>
#include <string>
#include <vector>
#include "ftxui/component/app.hpp"
#include <config.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string_view>
#if __has_include(<dirent.h>)
#include <dirent.h>
#endif
#if __has_include(<sys/stat.h>)
#include <sys/stat.h>
#endif
#if __has_include(<sys/time.h>)
#include <sys/time.h>
#endif
#if __has_include(<unistd.h>)
#include <unistd.h>
#endif
#if __has_include(<sys/cygwin.h>)
#include <sys/cygwin.h>
#endif
#include "src/common.hpp"
#include "src/intrf.hpp"
#include <cstdarg>
// #include "intrfn.hpp"
#include "askloc.hpp"
#include "src/dir.hpp"
#include "src/log.hpp"

using namespace ftxui;

static auto dir_aff_entry(const file_info_t &file_info) -> std::string;
static auto get_file_info(const std::filesystem::path &path) -> file_info_t;

#if defined(DJGPP) || defined(__OS2__)
void get_dos_drive_list(dir_list_t &list)
{
  for (char c : std::views::iota('a', 'z' + 1))
  {
    file_info_t new_drive;
    new_drive.name    = std::format("{}:/", c);
    new_drive.st_mode = LINUX_S_IFDIR | LINUX_S_IRWXUGO;
    list.push_back(new_drive);
  }
}
#endif

auto get_dir_list(std::filesystem::path dst_directory) -> dir_list_t
{
  dir_list_t dir_list;
#if defined(DJGPP) || defined(__OS2__)
  if (dst_directory.empty())
  {
    get_dos_drive_list(&dir_list);
  }
#endif
  if (!is_directory(dst_directory))
  {
    log_info("{} is not a directory", dst_directory.native());
    dst_directory = dst_directory.parent_path();
  }
  if (!is_directory(dst_directory))
  {
    dst_directory = std::filesystem::current_path();
  }
  if (!is_directory(dst_directory))
  {
    log_error("cannot access directory");
    return dir_list;
  }
  {

    file_info_t file_info = get_file_info(dst_directory);
    file_info.name        = ".";
    dir_list.push_back(file_info);

    if (!std::filesystem::equivalent(dst_directory, dst_directory / ".."))
    {
      file_info      = get_file_info(dst_directory / "..");
      file_info.name = "..";
      dir_list.push_back(file_info);
    }
  }
  {
    for (const auto &dir_entrie :
         std::filesystem::directory_iterator(dst_directory))
    {
#if !defined(__CYGWIN__) && !defined(DJGPP) && !defined(__MINGW32__) && \
    !defined(__OS2__) && !defined(WIN32)
      // hide filename beginning by '.'
      if (dir_entrie.is_directory() &&
          dir_entrie.path().filename().string().starts_with('.'))
        continue;
#endif
#ifdef __CYGWIN__
      if (dst_directory.root_directory().string() == "cygdrive" &&
          dir_entrie.is_directory() &&
          dir_entrie.path().filename().string().starts_with('.'))
        continue;
#endif
      {
        file_info_t file_info = get_file_info(dir_entrie.path());
        file_info.name        = dir_entrie.path().filename().string();
        dir_list.push_back(file_info);
      }
    }
  }
  dir_list.sort(filesort);

  return dir_list;
};

void ask_location(std::filesystem::path &dst_directory,
                  const unsigned int dst_size, std::string_view msg,
                  std::string_view src_dir)
{
  if (dst_directory.empty())
    dst_directory = std::filesystem::current_path();

  auto screen = App::Fullscreen();

  bool reload = true;
  std::shared_future<dir_list_t> dir_list =
      std::async(get_dir_list, dst_directory);

  int currentFile = 0;
  std::vector<std::string> menuEntries;

  auto dir_menu =
      Menu(&menuEntries, &currentFile, {.on_enter = [&] -> void {
        constexpr size_t posOfFileName = 51;
        auto newPath =
            dst_directory / menuEntries[currentFile].substr(posOfFileName);
        if (std::filesystem::is_directory(newPath))
        {
          dst_directory = newPath.lexically_normal();
          dir_list      = std::async(get_dir_list, dst_directory);
          reload        = true;
        }
      }}) |
      CatchEvent([&](const Event &event) -> bool {
        if (event == Event::q)
        {
          dst_directory = "";
          screen.Exit();
          return true;
        }
        if (event == Event::c)
        {
          screen.Exit();
          return true;
        }
        return false;
      });

  size_t frame = 0;

  Component root = Renderer(dir_menu, [&] -> Element {
    bool loaded =
        dir_list.valid() && dir_list.wait_for(std::chrono::milliseconds(10)) ==
                                std::future_status::ready;
    if (!loaded)
    {
      screen.RequestAnimationFrame();
      frame++;
    }
    else if (reload)
    {
      menuEntries = dir_list.get() | std::views::transform(&dir_aff_entry) |
                    std::ranges::to<std::vector<std::string>>();
      reload      = false;
    }
    return vbox({
        hflow({text("TestDisk++ "), bold(text(VERSION)),
               text(", Data Recovery Utility, "), text(TESTDISKDATE)}),
        separatorEmpty(),
        paragraph(msg),
        loaded
            ? vbox({
                  hbox({text("Keys: "),
                        vbox({
                            hflow({bold(text("Arrow")),
                                   text(" keys to select another directory")}),
                            hflow({bold(text("C")),
                                   text(" when the destination is correct")}),
                            hflow({bold(text("Q")), text(" to quit")}),
                        })}),
                  text(std::format("Directory {}", dst_directory.native())),
                  dir_menu->Render() | yframe,
              })
            : hflow({
                  bold(text("Directory listing in progress ")),
                  spinner(15, frame),
              }),
    });
  });

  screen.Loop(root);
}

static auto dir_aff_entry(const file_info_t &file_info) -> std::string
{
  char str[11];
  char datestr[80];
  set_datestr((char *)&datestr, sizeof(datestr), file_info.td_mtime);
  mode_string(file_info.st_mode, str);
  return std::format("{} {:5} {:5} {:9} {} {}", str, file_info.st_uid,
                     file_info.st_gid, file_info.st_size, datestr,
                     file_info.name);
}

static auto get_file_info(const std::filesystem::path &path) -> file_info_t
{
  struct stat file_stat;
  file_info_t file_info;
#ifdef HAVE_LSTAT
  if (lstat(path.c_str(), &file_stat) == 0)
#else
  if (stat(path.c_str(), &file_stat) == 0)
#endif
  {
    file_info.st_ino   = file_stat.st_ino;
    file_info.st_mode  = file_stat.st_mode;
    file_info.st_uid   = file_stat.st_uid;
    file_info.st_gid   = file_stat.st_gid;
    file_info.st_size  = file_stat.st_size;
    file_info.td_atime = file_stat.st_atime;
    file_info.td_mtime = file_stat.st_mtime;
    file_info.td_ctime = file_stat.st_ctime;
#if defined(DJGPP) || defined(__OS2__)
    /* If the C library doesn't use posix definition, st_mode need to be
     * fixed */
    if (S_ISDIR(file_info.st_mode))
      file_info.st_mode = LINUX_S_IFDIR | LINUX_S_IRWXUGO;
    else
      file_info.st_mode = LINUX_S_IFREG | LINUX_S_IRWXUGO;
#endif
#ifdef __CYGWIN__
    /* Fix Drive list */
    if (dst_directory.filename().string() == "cygdrive")
    {
      file_info.st_mode  = LINUX_S_IFDIR | LINUX_S_IRWXUGO;
      file_info.td_mtime = 0;
      file_info.st_uid   = 0;
      file_info.st_gid   = 0;
    }
#endif
  }
  return file_info;
}
