#include "src/ui/dir.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "src/common.hpp"
#include "src/dir.hpp"
#include "src/dir_common.hpp"
#include "src/setdate.hpp"
#include "src/ui/intrfn.hpp"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <format>
#include <future>
#include <utility>
#include <vector>

using namespace ftxui;

static auto is_inode_valid(const unsigned long new_inode,
                           const std::vector<unsigned long> &inode_known)
    -> bool
{
  if (new_inode < 2)
    return false;
  return std::ranges::all_of(inode_known, [&](unsigned long inode) -> bool {
    return new_inode != inode;
  });
}

static auto can_copy_dir(const file_info_t &current_file,
                         const std::vector<unsigned long> &inode_known) -> bool
{
  if (current_file.name == ".." || current_file.name == ".")
    return false;
  return is_inode_valid(current_file.st_ino, inode_known);
}

static auto do_file_copy(disk_t &disk, const partition_t &partition,
                         dir_data_t *&dir_data, const file_info_t &file,
                         unsigned int &copy_ok, unsigned int &copy_bad)
    -> copy_file_t
{
  dir_data->current_directory /= file.name;
  const copy_file_t res       = dir_data->copy_file(disk, partition, file);
  dir_data->current_directory = dir_data->current_directory.parent_path();

  if (res == CP_OK)
    copy_ok++;
  else
    copy_bad++;
  return res;
}

static auto do_dir_copy(disk_t &disk, const partition_t &partition,
                        dir_data_t *&dir_data, const file_info_t &dir,
                        unsigned int &copy_ok, unsigned int &copy_bad,
                        bool &stop) -> copy_dir_t
{
  dir_list_t dir_list;
  dir_data->get_dir(disk, partition, dir.st_ino, dir_list);

  char *dir_name = mkdir_local(dir_data->local_dir.string().c_str(),
                               dir_data->current_directory.string().c_str());

  assert(dir_name != nullptr && "mkdir_local failed");

  static std::vector<unsigned long> inode_known = {dir.st_ino};
  for (auto &file : dir_list)
  {
    copy_dir_t copy_stopped = copy_dir_t::FINISHED;

    if (LINUX_S_ISDIR(file.st_mode) != 0)
    {
      if (can_copy_dir(file, inode_known))
      {
        dir_data->current_directory /= file.name;
        copy_stopped = do_dir_copy(disk, partition, dir_data, file, copy_ok,
                                   copy_bad, stop);
        dir_data->current_directory = dir_data->current_directory.parent_path();
      }
    }
    else if (LINUX_S_ISREG(file.st_mode) != 0)
    {
      if (stop)
        copy_stopped = copy_dir_t::STOPPED;
      else
      {
        copy_file_t res =
            do_file_copy(disk, partition, dir_data, file, copy_ok, copy_bad);
        if (res == CP_NOSPACE)
          copy_stopped = copy_dir_t::NOSPACE;
      }
    }
    if (copy_stopped != copy_dir_t::FINISHED)
    {
      set_date(dir_name, dir.td_atime, dir.td_mtime);
      delete dir_name;
      inode_known.pop_back();
      return copy_stopped;
    }
  }
  dir_data->current_directory = dir_data->current_directory.parent_path();
  set_date(dir_name, dir.td_atime, dir.td_mtime);
  delete dir_name;
  inode_known.pop_back();
  return copy_dir_t::FINISHED;
}

auto copy_dir(const Component &root, disk_t &disk, const partition_t &partition,
              dir_data_t *&dir_data, const file_info_t &dir,
              unsigned int &copy_ok, unsigned int &copy_bad) -> copy_dir_t
{
  bool show_modal = true;
  auto screen     = App::Fullscreen();

  static bool stop = false;

  std::future<copy_dir_t> copy_stopped = std::async([&] -> copy_dir_t {
    return do_dir_copy(disk, partition, dir_data, dir, copy_ok, copy_bad, stop);
  });

  auto stopButton = Button("  Stop  ", [&] -> void {
    stop = true;
    screen.Exit();
  });

  auto dialog =
      Renderer(
          stopButton,
          [&] -> Element {
            if (copy_stopped.valid())
              screen.Exit();
            screen.RequestAnimationFrame();
            return vbox({
                text(std::format("Copying, please wait... {} ok, {} failed",
                                 copy_ok, copy_bad)),
                separator(),
                stopButton->Render(),
            });
          }
      ) |
      border | center;

  screen.Loop(root | Modal(dialog, &show_modal));

  copy_dir_t res = copy_stopped.get();
  if (res == copy_dir_t::STOPPED)
    display_message(root, std::format("Copy stopped! {} ok, {} failed", copy_ok,
                                      copy_bad));
  else if (res == copy_dir_t::NOSPACE)
    display_message(
        root, std::format("Copy stopped! {} ok, {} failed - Not enough space",
                          copy_ok, copy_bad)
    );
  else
    display_message(root, std::format("Copy done! {} ok, {} failed", copy_ok,
                                      copy_bad));

  return copy_dir_t::FINISHED;
}

auto copy_file(const Component &root, disk_t &disk,
               const partition_t &partition, dir_data_t *&dir_data,
               const file_info_t &file, unsigned int &copy_ok,
               unsigned int &copy_bad) -> copy_file_t
{
  bool show_modal = true;
  auto screen     = App::Fullscreen();

  std::future<copy_file_t> copy_stopped = std::async([&] -> copy_file_t {
    return do_file_copy(disk, partition, dir_data, file, copy_ok, copy_bad);
  });

  auto dialog =
      Renderer([&] -> Element {
        if (copy_stopped.valid())
          screen.Exit();
        screen.RequestAnimationFrame();
        return vbox({
            text(std::format("Copying '{}', please wait...", file.name)),
        });
      }) |
      border | center;

  screen.Loop(root | Modal(dialog, &show_modal));

  copy_file_t res = copy_stopped.get();
  switch (res)
  {
  case CP_OK:
    display_message(root, "Copy done!");
    break;
  case CP_NOSPACE:
    display_message(root, "Copy stopped! - Not enough space");
    break;
  case CP_NOMEM:
    display_message(root, "Copy stopped! - Out of memory");
    break;
  case CP_STAT_FAILED:
    display_message(root, "Copy stopped! - STAT_FAILED");
    break;
  case CP_READ_FAILED:
    display_message(root, "Copy stopped! - READ_FAILED");
    break;
  case CP_CREATE_FAILED:
    display_message(root, "Copy stopped! - CREATE_FAILED");
    break;
  case CP_OPEN_FAILED:
    display_message(root, "Copy stopped! - OPEN_FAILED");
    break;
  case CP_CLOSE_FAILED:
    display_message(root, "Copy stopped! - CLOSE_FAILED");
    break;
  default:
    std::unreachable();
  }

  return res;
}
