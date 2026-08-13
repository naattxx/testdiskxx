#pragma once
#include "common.hpp"
#include <optional>
#include <string>

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires disk_car->sector_size > 0;
  @ requires disk_car->geom.heads_per_cylinder > 0;
  @ requires \valid_function(disk_car->pread);
  @ decreases 0;
  @ ensures  valid_disk(disk_car);
  @*/
void hd_update_geometry(disk_t &disk_car, const int verbose);

/*@
  @ requires valid_list_disk(list_disk);
  @*/
void hd_update_all_geometry(list_disk_t &list_disk, const int verbose);

/*@
  @ requires valid_list_disk(list_disk);
  @ ensures  valid_list_disk(\result);
  @*/
void hd_parse(list_disk_t &list_disk, const int verbose, const int testdisk_mode);

/*@
  @ requires valid_read_string(device);
  @ ensures  \result!=\null ==> (0 < \result->geom.cylinders < 0x2000000000000);
  @ ensures  \result!=\null ==> (0 < \result->geom.heads_per_cylinder <= 255);
  @ ensures  \result!=\null ==> (0 < \result->geom.sectors_per_head <= 63);
  @ ensures  \result==\null || valid_disk(\result);
  @*/
std::optional<disk_t> file_test_availability(const char *device, const int verbose, const int testdisk_mode);

/*@
  @ requires \valid(disk_car);
  @ requires valid_disk(disk_car);
  @ requires 0 < disk_car->geom.heads_per_cylinder;
  @ requires 0 < disk_car->geom.sectors_per_head;
  @ requires 0 < disk_car->sector_size;
  @ ensures 0 < disk_car->geom.cylinders < 0x2000000000000;
  @ ensures  valid_disk(disk_car);
  @ assigns disk_car->disk_real_size, disk_car->geom.cylinders, disk_car->disk_size;
  @*/
void update_disk_car_fields(disk_t &disk_car);

/*@
  @ requires \valid(disk);
  @ requires valid_disk(disk);
  @ requires \freeable(disk);
  @ requires valid_disk(disk);
  @*/
void generic_clean(disk_t &disk);
