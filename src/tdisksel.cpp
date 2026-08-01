#include "tdisksel.hpp"
#include "cpptui.hpp"
#include "src/log.hpp"
#include <config.h>

using namespace cpptui;

void testdisk_disk_selection(App &app, int verbose, bool dump, list_disk_t &list_disk, bool save_header)
{
    auto root = std::make_shared<Vertical>();

    auto aboutBuild = std::make_shared<Label>(
        StyledText("TestDisk++ ").bold(VERSION).add(", Datah Recovery Utility, " TESTDISKDATE));
    auto author = std::make_shared<Label>("naattxx <grenier@cgsecurity.org>");
    auto website = std::make_shared<Label>("https://www.github.com/");

    auto noWarranty = std::make_shared<Paragraph>("TestDisk is free software, and comes with ABSOLUTELY NO WARRANTY.");
    noWarranty->first_line_indent = 2;

    root->add(aboutBuild);
    root->add(author);
    root->add(website);
    root->add(std::make_shared<VerticalSpacer>(1));
    root->add(noWarranty);

    if (list_disk.empty())
    {
        log_critical("No disk found");
        root->add(std::make_shared<Label>("No hard disk found"));
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
        root->add(std::make_shared<Paragraph>("You need to be administrator to use TestDisk++.\n"
                                              "select TestDisk++, right-click and choose \"Run as administrator\"."));
#elif defined(DJGPP)
#elif __has_include("unistd.h")
        root->add(std::make_shared<Static>("You need to be root to use TestDisk++."));
#endif
        root->add(std::make_shared<VerticalSpacer>(1));
    }
    else
    {

        auto SerialN = std::make_shared<Static>("");

        for (auto &disk : list_disk)
        {
            auto diskBtn = std::make_shared<Button>(disk->description_short(disk));
            diskBtn->alignment = Alignment::Left;
            diskBtn->on_hover = [disk, SerialN](bool hover) {
                if (hover && disk->serial_no)
                {
                    SerialN->set_text(StyledText("Serial number: ").colored(disk->serial_no, Color::Green()));
                }
                else
                {
                    SerialN->set_text("");
                }
            };
            root->add(diskBtn);
        }

        root->add(std::make_shared<VerticalSpacer>());

        root->add(SerialN);

#if __has_include("unistd.h") && !defined(__CYGWIN__) && !defined(__MINGW32__) && !defined(DJGPP)
        if (geteuid() != 0)
        {
            auto noRootWarn = std::make_shared<Static>(StyledText("Note: ").colored_bold(
                "Some disks won't appear unless you are root user.", Theme::current().warning));
            root->add(noRootWarn);
        }
#endif

        auto capacityWarning = std::make_shared<Paragraph>(
            "Disk capacity must be correctly detected for a successful recovery.\n"
            "If a disk listed above has an incorrect size, check HD jumper settings and BIOS "
            "detection, and install the latest OS patches and disk drivers.");

        root->add(capacityWarning);
    }

    app.run(root);
}
