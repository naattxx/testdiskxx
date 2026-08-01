#include "common.hpp"
#include "config.h"
#include "fnctdsk.hpp"
#include "hdaccess.hpp"
#include "hdcache.hpp"
#include "log.hpp"
#include "src/autoset.hpp"
#include "src/hidden.hpp"
#include "src/intrface.hpp"
#include "src/partauto.hpp"
#include "src/tdisksel.hpp"
#include "utils.hpp"
#include "tlog.hpp"
#include <args.hxx>
#include <cpptui.hpp>
#include <ctime>
#include <iostream>
#include <string>

using namespace cpptui;

static void display_version(void)
{
    std::cout << "\n"
                 "Version: " VERSION "\n"
                 "Compiler: " COMPILER_ID "\n"
#ifdef RECORD_COMPILATION_DATE
              << "Compilation date: " << get_compilation_date() << "\n"
#endif
              << "cpptui.hpp: " << cpptui::VERSION_MAJOR << '.' << cpptui::VERSION_MINOR << '.' << cpptui::VERSION_PATCH
              << ", "
              << "args.hxx: " << ARGS_VERSION << '\n'
#ifdef HAVE_ICONV
              << "iconv support: yes\n"
#else
              << "iconv support: no\n"
#endif
        ; // << "OS: " << get_os() << '\n';
}

static int display_disk_list(list_disk_t list_disk, const int testdisk_mode, const int create_backup, const int safe,
                             const int saveheader, const UNIT unit, const int verbose)
{
    std::cout << "Please wait...\n";
    /* Scan for available device only if no device or image has been supplied in parameter */
    if (list_disk.empty())
        hd_parse(list_disk, verbose, testdisk_mode);
    if (list_disk.empty())
    {
        std::cout << "No disk detected.\n";
#if __has_include(<unistd.h>) && !defined(__CYGWIN__) && !defined(__MINGW32__) && !defined(DJGPP)
        if (geteuid() != 0)
        {
            std::cout << "You need to be root to use TestDisk." << std::endl;
        }
#endif
        return 1;
    }

    /* Activate the cache */
    for (disk_t *disk : list_disk)
        disk = new_diskcache(disk, testdisk_mode);
    if (safe == 0)
        hd_update_all_geometry(list_disk, verbose);
    for (disk_t *disk : list_disk)
    {
        const int hpa_dco = is_hpa_or_dco(disk);
        std::cout << disk->description(disk) << '\n';
        std::cout << "Sector size: " << disk->sector_size << '\n';
        if (disk->model != nullptr)
            std::cout << "Model: " << disk->model;
        if (disk->serial_no != nullptr)
            std::cout << ", S/N: " << disk->serial_no;
        if (disk->fw_rev != nullptr)
            std::cout << ", FW: " << disk->fw_rev;
        std::cout << '\n';
        if (hpa_dco != 0)
        {
            if (disk->sector_size != 0)
                std::cout << "size       " << (long long unsigned)(disk->disk_real_size / disk->sector_size)
                          << " sectors\n";
            if (disk->user_max != 0)
                std::cout << "user_max   " << (long long unsigned)disk->user_max << " sectors\n";
            if (disk->native_max != 0)
                std::cout << "native_max " << (long long unsigned)(disk->native_max + 1) << " sectors\n";
            if (disk->dco != 0)
                std::cout << "dco        " << (long long unsigned)(disk->dco + 1) << " sectors\n";
            if (hpa_dco & 1)
                std::cout << "Host Protected Area (HPA) present.\n";
            if (hpa_dco & 2)
                std::cout << "Device Configuration Overlay (DCO) present.\n";
        }
        std::cout << '\n';
    }

    for (disk_t* disk : list_disk)
    {
        autodetect_arch(disk, nullptr);
        if (unit == UNIT::DEFAULT)
            autoset_unit(disk);
        else
            disk->unit = unit;
        interface_list(disk, verbose, saveheader, create_backup);
        std::cout << '\n';
    }
    delete_list_disk(list_disk);
    return 0;
}

int main(int argc, char **argv)
{
    TD_LOG create_log{TD_LOG::NONE};
    bool log_opened = false;
    int verbose = 0;
    list_disk_t list_disk;
    int testdisk_mode = TESTDISK_O_RDWR | TESTDISK_O_READAHEAD_8K;
    UNIT unit = UNIT::DEFAULT;

    args::ArgumentParser parser("TestDisk " VERSION ", Data Recovery Utility, " TESTDISKDATE
                                "\nChristophe GRENIER <grenier@cgsecurity.org>\n"
                                "https://www.cgsecurity.org\n",
                                "TestDisk checks and recovers lost partitions\n"
                                "It works with :\n"
                                "- BeFS (BeOS)                           - BSD disklabel (Free/Open/Net BSD)\n"
                                "- CramFS, Compressed File System        - DOS/Windows FAT12, FAT16 and FAT32\n"
                                "- XBox FATX                             - Windows exFAT\n"
                                "- HFS, HFS+, Hierarchical File System   - JFS, IBM's Journaled File System\n"
                                "- Linux btrfs                           - Linux ext2, ext3 and ext4\n"
                                "- Linux GFS2                            - Linux LUKS\n"
                                "- Linux Raid                            - Linux Swap\n"
                                "- LVM, LVM2, Logical Volume Manager     - Netware NSS\n"
                                "- Windows NTFS                          - ReiserFS 3.5, 3.6 and 4\n"
                                "- Sun Solaris i386 disklabel            - UFS and UFS2 (Sun/BSD/...)\n"
                                "- XFS, SGI's Journaled File System      - Wii WBFS\n"
                                "- Sun ZFS\n");
    args::HelpFlag help(parser, "help", "Display this help menu", {"h", "?", "help"});
    args::Flag dump(parser, "dump", "", {"dump"});
    args::ValueFlag<std::string> log_name(parser, "file.dd|file.e01|device", "Specify the log file name", {"logname"},
                                          "testdisk.log");
    args::Flag no_log(parser, "no-log", "Disable logging", {"nolog"});
    args::Flag log(parser, "log", "Create a testdisk.log file", {"log"});
    args::Flag debug(parser, "debug", "Add debug information", {"debug"});
    args::Flag all(parser, "all", "", {"all"});
    args::Flag create_backup(parser, "backup", "", {"backup"});
    args::Flag direct(parser, "direct", "", {"direct"});
    args::Flag version(parser, "version", "", {"v", "version"});
    args::Flag list(parser, "list", "Display current partitions", {"l", "list"});
    args::Flag list_unit(parser, "list-unit", "", {"lu"});
    args::Flag nosetlocale(parser, "no-setlocale", "", {"nosetlocale"});
    args::Flag safe(parser, "safe", "", {"safe"});
    args::Flag save_header(parser, "save-header", "", {"saveheader"});
    args::ValueFlag<std::string> cmd(parser, "cmd", "Specify the command to execute", {"cmd"});
    try
    {
        parser.LongPrefix("/");
        parser.ShortPrefix("/");
        parser.LongSeparator(" ");
        parser.ParseCLI(argc, argv);
    }
    catch (const args::Completion &e)
    {
        std::cout << e.what();
        return 0;
    }
    catch (const args::Help &)
    {
        std::cout << parser;
        return 0;
    }
    catch (const args::ParseError &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    if (version)
    {
        display_version();
        return 0;
    }
    if (no_log)
        create_log = TD_LOG::NONE;
    if (log)
    {
        if (create_log == TD_LOG::NONE)
            create_log = TD_LOG::APPEND;
        if (!log_opened)
            log_opened = log_open(args::get(log_name), create_log);
    }
    if (debug)
    {
        spdlog::set_level(spdlog::level::debug);
        verbose++;
        if (create_log == TD_LOG::NONE)
            create_log = TD_LOG::APPEND;
        if (!log_opened)
            log_opened = log_open(args::get(log_name), create_log);
    }
    if (all)
        testdisk_mode |= TESTDISK_O_ALL;
    if (direct)
        testdisk_mode |= TESTDISK_O_DIRECT;
    if (list || list_unit)
    {
        if (list_unit)
            unit = UNIT::SECTOR;
        const int res = display_disk_list(list_disk, testdisk_mode, create_backup, safe, save_header, unit, verbose);
        return res;
    }
    if (create_log != TD_LOG::NONE && !log_opened)
        log_opened = log_open(args::get(log_name), create_log);
    App app;

    Theme::set_theme(Theme::Dark());
    app.register_exit_key('q');
    if (argc == 1 && create_log == TD_LOG::NONE)
    {
        verbose = 1;


        create_log = ask_testdisk_log_creation(app);
        if (create_log == TD_LOG::CREATE || create_log == TD_LOG::APPEND)
            log_opened = log_open(args::get(log_name), create_log);

        if (create_log != TD_LOG::NONE && !log_opened)
        {
            // TODO: implament ask_log_location()
            std::cout << "TODO: implament ask_log_location()\n";
            return 1;
        }
    }
    {
        std::time_t my_time;
        my_time = time(NULL);
        log_info("{}", ctime(&my_time));
    }
    {
        std::string cmd("Command line:");
        for (int i{0}; i < argc; i++)
            cmd.append(" ").append(argv[i]);
        log_info("{}", cmd.c_str());
    }
    log_info("TestDisk {}, Data Recovery Utility, {}\nChristophe GRENIER "
             "<grenier@cgsecurity.org>\nhttps://www.cgsecurity.orgs",
             VERSION, TESTDISKDATE);
    // log_info("OS: {}" , get_os());
    log_info("Compiler: {}", COMPILER_ID);

#ifdef RECORD_COMPILATION_DATE
    log_info("Compilation date: {}", get_compilation_date());
#endif
    log_info("cpptui.hpp: {}.{}.{}, args.hxx: {}", cpptui::VERSION_MAJOR, cpptui::VERSION_MINOR, cpptui::VERSION_PATCH,
             ARGS_VERSION);

    if (!isAdmin())
    {
        log_warning("User is not root!");
    }

    /* Scan for available device only if no device or image has been supplied in parameter */
    if (list_disk.empty())
        hd_parse(list_disk, verbose, testdisk_mode);
    /* Activate the cache */
    for (disk_t* disk : list_disk)
        disk = new_diskcache(disk, testdisk_mode);

    if(safe==0)
        hd_update_all_geometry(list_disk, verbose);
    log_disk_list(list_disk);

    testdisk_disk_selection(app, verbose, dump, list_disk,save_header);

    return 0;
}
