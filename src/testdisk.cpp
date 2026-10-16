#include "common.hpp"
#include "config.h"
#include "fnctdsk.hpp"
#include "ftxui/component/app.hpp"
#include "hdaccess.hpp"
#include "hdcache.hpp"
#include "log.hpp"
#include "src/ui/intrface.hpp"
#include "src/ui/intrfn.hpp"
#include "ui/tdisksel.hpp"
#include "utils.hpp"
#include "ui/tlog.hpp"
#include <args.hxx>
#include <chrono>
#include <cstring>
#include <format>
#include <iostream>
#include <optional>
#include <print>
#include <string>

using namespace ftxui;

static void display_version() noexcept
{
    std::cout << "\n"
                 "Version: " VERSION "\n"
                 "Compiler: " COMPILER_ID "\n"
#ifdef RECORD_COMPILATION_DATE
              << "Compilation date: " << get_compilation_date() << "\n"
#endif
              << "ftxui: " << FTXUI_VERSION
              << ", "
              << "args.hxx: " << ARGS_VERSION << '\n'
#ifdef HAVE_ICONV
              << "iconv support: yes\n"
#else
              << "iconv support: no\n"
#endif
        ; // << "OS: " << get_os() << '\n';
}

static auto display_disk_list(list_disk_t list_disk, const unsigned int testdisk_mode, const int create_backup, const int safe,
                             const int saveheader, const UNIT unit, const int verbose) noexcept -> int
{
    std::println("Please wait...");
    /* Scan for available device only if no device or image has been supplied in parameter */
    if (list_disk.empty())
        hd_parse(list_disk, verbose, testdisk_mode);
    if (list_disk.empty())
    {
        std::println("No disk detected.");
        if (!isAdmin())
        {
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
            std::println("You need to be Administrator to use TestDisk.");
#elifdef __linux__
          std::println("You need to be root to use TestDisk.");
#endif
        }
        return 1;
    }

    /* Activate the cache */
    for (disk_t &disk : list_disk)
        disk = new_diskcache(disk, testdisk_mode);
    if (safe == 0)
        hd_update_all_geometry(list_disk);
    for (disk_t &disk : list_disk)
    {
        const unsigned int hpa_dco = disk.is_hpa_or_dco();
        std::println("{}", disk.description(disk));
        std::println("Sector size: {}", disk.sector_size);
        if (!disk.model.empty())
            std::print("Model: {}", disk.model);
        if (!disk.serial_no.empty())
            std::print(", S/N: {}", disk.serial_no);
        if (!disk.fw_rev.empty())
            std::print(", FW: {}", disk.fw_rev);
        std::println();
        if (hpa_dco != 0)
        {
            if (disk.sector_size != 0)
                std::println("size       {} sectors", disk.disk_real_size / disk.sector_size);
            if (disk.user_max != 0)
                std::println("user_max   {} sectors", disk.user_max);
            if (disk.native_max != 0)
                std::println("native_max {} sectors", disk.native_max + 1);
            if (disk.dco != 0)
                std::println("dco        {} sectors", disk.dco + 1);
            if (hpa_dco & 1u)
                std::println("Host Protected Area (HPA) present.");
            if (hpa_dco & 2u)
                std::println("Device Configuration Overlay (DCO) present.");
        }
        std::println();
    }

    for (disk_t& disk : list_disk)
    {
        disk.autodetect_arch(nullptr);
        if (unit == UNIT::DEFAULT)
            disk.autoset_unit();
        else
            disk.unit = unit;
        interface_list(disk, verbose, saveheader, create_backup);
        std::println();
    }
    delete_list_disk(list_disk);
    return 0;
}

auto main(int argc, char **argv) -> int
{
    TD_LOG create_log{TD_LOG::NONE};
    bool log_opened = false;
    int verbose = 0;
    list_disk_t list_disk;
    unsigned int testdisk_mode = TESTDISK_O::RDWR | TESTDISK_O::READAHEAD_8K;
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
    args::Positional<std::string> path(parser, "path", "file or disk path");
    try
    {
        parser.ParseCLI(argc, argv);

        if (cmd || path) {
            std::optional<disk_t> disk_car=file_test_availability(path.Get().c_str(), verbose, testdisk_mode);
            if (disk_car)
                insert_new_disk(list_disk, *disk_car);
            else
                throw args::ParseError(std::format("Unable to open file or device \"{}\": {}", path.Get(), strerror(errno)));
        }
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
    catch (const args::Error &e)
    {
        std::cerr << e.what() << '\n';
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
        testdisk_mode |= TESTDISK_O::ALL;
    if (direct)
        testdisk_mode |= TESTDISK_O::DIRECT;
    if (list || list_unit)
    {
        if (list_unit)
            unit = UNIT::SECTOR;
        const int res = display_disk_list(list_disk, testdisk_mode, create_backup, safe, save_header, unit, verbose);
        return res;
    }
    if(!nosetlocale)
    {
      const char *locale;
      locale = setlocale(LC_ALL, "");
      if (locale==nullptr) {
        locale = setlocale(LC_ALL, nullptr);
        log_error("Failed to set locale, using default '{}'.", locale);
      } else {
        log_info("Using locale '{}'.", locale);
      }
    }
    if (create_log != TD_LOG::NONE && !log_opened)
        log_opened = log_open(args::get(log_name), create_log);
    App app = App::Fullscreen();

    if (argc == 1 && create_log == TD_LOG::NONE)
    {
        verbose = 1;

        create_log = ask_testdisk_log_creation(app);
        if (create_log == TD_LOG::CREATE || create_log == TD_LOG::APPEND)
            log_opened = log_open(args::get(log_name), create_log);

        if (create_log != TD_LOG::NONE && !log_opened)
        {
            std::string filename = ask_log_location(args::get(log_name));
            if(filename.empty())
                create_log = TD_LOG::NONE;
            else
                log_opened = log_open(filename, create_log);
        }
    }
    try {
    log_info(std::format("{}", std::chrono::system_clock::now()));
    }
    catch(...) {
      log_warning("Can't format time");
    }
    {
        std::string cmd("Command line:");
        for (int i{0}; i < argc; i++)
            cmd.append(" ").append(argv[i]);
        log_info("{}", cmd);
    }
    log_info("TestDisk " VERSION ", Data Recovery Utility, " TESTDISKDATE "\nChristophe GRENIER "
             "<grenier@cgsecurity.org>\nhttps://www.cgsecurity.orgs"
            );
    // log_info("OS: {}" , get_os());
    log_info("Compiler: {}", COMPILER_ID);

#ifdef RECORD_COMPILATION_DATE
    log_info("Compilation date: {}", get_compilation_date());
#endif
    log_info("ftxui: " FTXUI_VERSION ", args.hxx: " ARGS_VERSION);

    if (!isAdmin())
    {
        log_warning("User is not root!");
    }

    /* Scan for available device only if no device or image has been supplied in parameter */
    if (list_disk.empty())
        hd_parse(list_disk, verbose, testdisk_mode);
    /* Activate the cache */
    for (disk_t& disk : list_disk)
        disk = new_diskcache(disk, testdisk_mode);

    if(safe==0)
        hd_update_all_geometry(list_disk);
    log_disk_list(list_disk);

    testdisk_disk_selection(app, verbose, dump, list_disk,save_header);

    return 0;
}
