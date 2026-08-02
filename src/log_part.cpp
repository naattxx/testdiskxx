#include <stdio.h>
// #include "types.h"
#include "common.hpp"
#include "fnctdsk.hpp"
#include "intrf.hpp" /* aff_part_aux */
#include "log.hpp"
#include "log_part.hpp"
#include "src/log.hpp"

void log_partition(const disk_t &disk, const partition_t *partition)
{
    const char *msg;
    char buffer_part_size[100];
    msg = aff_part_aux(AFF_PART_ORDER | AFF_PART_STATUS, disk, partition);
    log_info("{}", msg);
    size_to_unit(partition->part_size, buffer_part_size);
    if (partition->info[0] != '\0')
        log_info("\n     %s, %s", partition->info, buffer_part_size);
    log_info("\n");
}

void log_all_partitions(const disk_t &disk, const list_part_t *list_part)
{
    const list_part_t *element;
    for (element = list_part; element != NULL; element = element->next)
        log_partition(disk, element->part);
}
