#ifndef _LOG_PART_H
#define _LOG_PART_H
#include "src/common.hpp"

/*@
  @ requires \valid_read(disk);
  @ requires valid_disk(disk);
  @ requires \valid_read(partition);
  @*/
void log_partition(const disk_t *disk, const partition_t *partition);

/*@
  @ requires \valid_read(disk);
  @ requires valid_disk(disk);
  @ requires valid_list_part(list_part);
  @*/
void log_all_partitions(const disk_t *disk, const list_part_t *list_part);

#endif
