#include <cstdio>
#include <format>
#include <string>
#include "common.hpp"
#include "fnctdsk.hpp"
#include "intrf.hpp" /* aff_part_aux */
#include "log.hpp"
#include "log_part.hpp"
#include "src/log.hpp"

void log_partition(const disk_t &disk, const partition_t &partition)
{
    const auto msg = aff_part_aux(AFF_PART_ORDER | AFF_PART_STATUS, disk, partition);
    log_info(std::format("{}", msg));
    const std::string part_size_str = size_to_unit(partition.part_size);
    if (partition.info[0] != '\0')
        log_info("\n     {}, {}", partition.info, part_size_str);
}

void log_all_partitions(const disk_t &disk, const list_part_t &list_part)
{
    for (const partition_t &element : list_part)
        log_partition(disk, element);
}
