#ifndef _UI_ADV_H
#define _UI_ADV_H

#include "src/common.hpp"

/*@
  @ requires \valid(disk_car);
  @*/
void interface_adv(disk_t &disk_car, const int verbose, const bool dump_ind, const bool expert);

#endif
