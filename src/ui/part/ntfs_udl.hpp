#pragma once

#include "src/common.hpp"
#include "src/dir_common.hpp"

void ntfs_undelete_menu(disk_t &disk, const partition_t &partition,
                        dir_data_t *&ntfs_dir_data, dir_list_t &dir_list);
