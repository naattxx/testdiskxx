#include "common.hpp"

void disk_t::set_cylinders_from_size_up()
{
  geom.cylinders=(disk_size / sector_size +
      geom.sectors_per_head * geom.heads_per_cylinder - 1) /
    (geom.sectors_per_head * geom.heads_per_cylinder);
}

auto disk_t::set_sector_size(const unsigned int sector_size) -> int
{
  /* Using 3*512=1536 as sector size and */
  /* 63/3=21 for number of sectors is an easy way to test */
  /* MS Backup internal blocksize is 256 bytes */
  switch(sector_size)
  {
    case 1:
    case 256:
    case 512:
    case 1024:
    case 3*512:
    case 2048:
    case 4096:
    case 8192:
      this->sector_size = sector_size;
      return 0;
    default:
      return 1;
  }
}
