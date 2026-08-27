#ifndef _GODMODE_H
#define _GODMODE_H

#include "src/common.hpp"

auto interface_recovery(disk_t &disk_car, const list_part_t &list_part_org,
                        const int verbose, const bool dump, const bool align,
                        const bool ask_part_order, const bool expert) -> int;

#endif
