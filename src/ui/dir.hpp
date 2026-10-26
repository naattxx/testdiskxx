#pragma once
#include "ftxui/component/app.hpp"
#include "src/common.hpp"
#include "src/dir_common.hpp"
#include <cstdint>
#include <ftxui/ftxui.hpp>

enum class copy_dir_t : uint8_t
{
  FINISHED = 0,
  STOPPED  = 1,
  NOSPACE  = 2,
};

auto copy_dir(const ftxui::Component &root, disk_t &disk,
              const partition_t &partition, dir_data_t *&dir_data,
              const file_info_t &dir, unsigned int &copy_ok,
              unsigned int &copy_bad) -> copy_dir_t;

auto copy_file(const ftxui::Component &root, disk_t &disk,
               const partition_t &partition, dir_data_t *&dir_data,
               const file_info_t &file, unsigned int &copy_ok,
               unsigned int &copy_bad) -> copy_file_t;
