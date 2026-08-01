#include "tdisksel.hpp"
#include "cpptui.hpp"
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

    auto note = std::make_shared<Static>("");

    for (auto &disk : list_disk)
    {
        auto diskBtn = std::make_shared<Button>(disk->description_short(disk));
        diskBtn->alignment = Alignment::Left;
        diskBtn->on_hover = [disk, note](bool hover) {
            if (hover && disk->serial_no)
            {
                note->set_text(StyledText("Note: ")
                                   .colored("Serial number ", Color::Green())
                                   .colored(disk->serial_no, Color::Green()));
            }
            else
            {
                note->set_text("");
            }
        };
        root->add(diskBtn);
    }

    root->add(std::make_shared<VerticalSpacer>());

    auto capacityWarning =
        std::make_shared<Paragraph>("Disk capacity must be correctly detected for a successful recovery.\n"
                                    "If a disk listed above has an incorrect size, check HD jumper settings and BIOS "
                                    "detection, and install the latest OS patches and disk drivers.");

    root->add(note);
    root->add(capacityWarning);

    app.run(root);
}
