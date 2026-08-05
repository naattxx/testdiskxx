/*

    File: savehdr.c

    Copyright (C) 2004-2007 Christophe GRENIER <grenier@cgsecurity.org>

    This software is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */
#include <string.h>
#include <time.h>
#ifdef __linux__
#include <sys/time.h>
#endif
#include <stdio.h>
#include <stdlib.h>
// #include <errno.h>
// #include "types.h"
#include "common.hpp"
#include "fnctdsk.hpp" /* get_LBA_part */
#include "log.hpp"
#include "savehdr.hpp"
#define BACKUP_MAXSIZE 5120

int save_header(disk_t &disk_car, const partition_t *partition, const int verbose)
{
    unsigned char *buffer;
    FILE *f_backup;
    int res = 0;
    if (verbose > 1)
    {
        // log_trace("save_header\n");
    }
    f_backup = fopen("header.log", "ab");
    if (!f_backup)
    {
        log_critical("Can't create header.log file: {}\n", strerror(errno));
        return -1;
    }
    buffer = new unsigned char[256 * DEFAULT_SECTOR_SIZE];
    memset(buffer, 0, DEFAULT_SECTOR_SIZE);
    {
        char status = 'D';
        switch (partition->status)
        {
        case STATUS_PRIM:
            status = 'P';
            break;
        case STATUS_PRIM_BOOT:
            status = '*';
            break;
        case STATUS_EXT:
            status = 'E';
            break;
        case STATUS_EXT_IN_EXT:
            status = 'X';
            break;
        case STATUS_LOG:
            status = 'L';
            break;
        case STATUS_DELETED:
            status = 'D';
            break;
        }
        snprintf((char *)buffer, 256 * DEFAULT_SECTOR_SIZE, "%s\n%2u %c Sys=%02X %5u %3u %2u %5u %3u %2u %10lu\n",
                 disk_car.description(disk_car), partition->order, status,
                 (disk_car.arch->get_part_type != NULL ? disk_car.arch->get_part_type(partition) : 0),
                 offset2cylinder(disk_car, partition->part_offset), offset2head(disk_car, partition->part_offset),
                 offset2sector(disk_car, partition->part_offset),
                 offset2cylinder(disk_car, partition->part_offset + partition->part_size - disk_car.sector_size),
                 offset2head(disk_car, partition->part_offset + partition->part_size - disk_car.sector_size),
                 offset2sector(disk_car, partition->part_offset + partition->part_size - disk_car.sector_size),
                 (unsigned long)(partition->part_size / disk_car.sector_size));
    }
    if (fwrite(buffer, DEFAULT_SECTOR_SIZE, 1, f_backup) != 1)
        res = -1;
    if (res >= 0 && disk_car.pread(disk_car, buffer, 256 * DEFAULT_SECTOR_SIZE, partition->part_offset) !=
                        256 * DEFAULT_SECTOR_SIZE)
        res = -1;
    if (res >= 0 && fwrite(buffer, DEFAULT_SECTOR_SIZE, 256, f_backup) != 256)
        res = -1;
    fclose(f_backup);
    delete[] (buffer);
    return res;
}

backup_disk_list_t partition_load(const disk_t &disk_car, const int verbose)
{
    FILE *f_backup;
    char *buffer;
    char *pos = NULL;
    int taille;
    backup_disk_t *new_backup = NULL;
    backup_disk_list_t list_backup;

    if (verbose > 1)
    {
        // log_trace("partition_load\n");
    }
    f_backup = fopen("backup.log", "r");
    if (!f_backup)
    {
        log_error("Can't open backup.log file: {}\n", strerror(errno));
        return list_backup;
    }
    buffer = new char[BACKUP_MAXSIZE];
    taille = fread(buffer, 1, BACKUP_MAXSIZE, f_backup);
    buffer[(taille < BACKUP_MAXSIZE ? taille : BACKUP_MAXSIZE - 1)] = '\0';
    if (verbose > 1)
    {
        log_info("partition_load backup.log size={}\n", taille);
    }
    for (pos = buffer; pos < buffer + taille; pos++)
    {
        if (*pos == '\n')
        {
            *pos = '\0';
        }
    }
    pos = buffer;
    while (pos != NULL && pos < buffer + taille)
    {
        if (*pos == '#')
        {
            pos++;
            if (verbose > 1)
            {
                // log_verbose("new disk: %s\n",pos);
            }
            if (new_backup != NULL)
                list_backup.push_front(new_backup);
            new_backup = new backup_disk_t;
            new_backup->description[0] = '\0';
            new_backup->my_time = strtol(pos, &pos, 10);
            if (pos != NULL)
            {
                strncpy(new_backup->description, ++pos, sizeof(new_backup->description));
                new_backup->description[sizeof(new_backup->description) - 1] = '\0';
            }
        }
        else if (new_backup != NULL)
        {
            partition_t *new_partition = new partition_t(disk_car.arch);
            char status;
            unsigned int part_type;
            unsigned long part_size;
            unsigned long part_offset;
            if (verbose > 1)
            {
                // log_verbose("new partition\n");
            }
            if (sscanf(pos, "%2u : start=%10lu, size=%10lu, Id=%02X, %c\n", &new_partition->order, &part_offset,
                       &part_size, &part_type, &status) == 5)
            {
                new_partition->part_offset = (uint64_t)part_offset * disk_car.sector_size;
                new_partition->part_size = (uint64_t)part_size * disk_car.sector_size;
                if (disk_car.arch->set_part_type != NULL)
                    disk_car.arch->set_part_type(new_partition, part_type);
                switch (status)
                {
                case 'P':
                    new_partition->status = STATUS_PRIM;
                    break;
                case '*':
                    new_partition->status = STATUS_PRIM_BOOT;
                    break;
                case 'L':
                    new_partition->status = STATUS_LOG;
                    break;
                default:
                    new_partition->status = STATUS_DELETED;
                    break;
                }
                {
                    int insert_error = 0;
                    insert_new_partition(new_backup->list_part, new_partition, 0, &insert_error);
                    if (insert_error > 0)
                        delete (new_partition);
                }
            }
            else
            {
                log_critical("partition_load: sscanf failed\n");
                delete (new_partition);
                pos = NULL;
            }
        }
        if (pos != NULL)
        {
            while (*pos != '\0' && pos < buffer + taille)
                pos++;
            pos++;
        }
    }
    if (new_backup != NULL)
        list_backup.push_front(new_backup);
    fclose(f_backup);
    delete[] (buffer);
    return list_backup;
}

int partition_save(disk_t &disk_car, const list_part_t &list_part, const int verbose)
{
    FILE *f_backup;
    if (verbose > 0)
    {
        // log_trace("partition_save\n");
    }
    f_backup = fopen("backup.log", "a");
    if (!f_backup)
    {
        log_critical("Can't create backup.log file: {}\n", strerror(errno));
        return -1;
    }
    fprintf(f_backup, "#%lu %s\n", (unsigned long int)time(NULL), disk_car.description(disk_car));
    for (const partition_t *partition : list_part)
    {
        char status = 'D';
        switch (partition->status)
        {
        case STATUS_PRIM:
            status = 'P';
            break;
        case STATUS_PRIM_BOOT:
            status = '*';
            break;
        case STATUS_EXT:
            status = 'E';
            break;
        case STATUS_EXT_IN_EXT:
            status = 'X';
            break;
        case STATUS_LOG:
            status = 'L';
            break;
        case STATUS_DELETED:
            status = 'D';
            break;
        }
        fprintf(f_backup, "%2u : start=%9lu, size=%9lu, Id=%02X, %c\n",
                (partition->order < 100 ? partition->order : 0),
                (unsigned long)(partition->part_offset / disk_car.sector_size),
                (unsigned long)(partition->part_size / disk_car.sector_size),
                (disk_car.arch->get_part_type != NULL ? disk_car.arch->get_part_type(partition) : 0), status);
    }
    fclose(f_backup);
    return 0;
}
