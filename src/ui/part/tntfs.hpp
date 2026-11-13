#pragma once
#include "src/common.hpp"

auto ntfs_boot_sector_menu(disk_t &disk, partition_t &partition, const int verbose,
                      const unsigned int expert) -> bool;
