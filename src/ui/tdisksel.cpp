#include "tdisksel.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"
#include "src/log.hpp"
#include "src/utils.hpp"
#include <config.h>
#include <string_view>
#if __has_include("unistd.h") && !defined(__CYGWIN__) &&   \
                  !defined(__MINGW32__) && !defined(DJGPP)
#include <unistd.h>
#endif

using namespace ftxui;

void testdisk_disk_selection(App &app, int verbose, bool dump,
                             list_disk_t &list_disk, bool save_header)
{
  auto diskList = Container::Vertical({});
  auto SerialN  = emptyElement();
  auto root     = Renderer(diskList, [&]() {
    return vbox({
        hflow({text("TestDisk++ "), bold(text(VERSION)),
               text(", Data Recovery Utility, "), text(TESTDISKDATE)}),
        text("naattxx <grenier@cgsecurity.org>"),
        text("https://www.github.com/"),
        separatorEmpty(),
        paragraph("  TestDisk is free software, and comes with ABSOLUTELY NO "
                  "WARRANTY."),
        separatorEmpty(),
        text("Select a media using arrow keys and press Enter:"),
        diskList->Render() | vscroll_indicator | yframe,
        filler(),
        SerialN,
#if __has_include("unistd.h") && !defined(__CYGWIN__) &&   \
                  !defined(__MINGW32__) && !defined(DJGPP)
        (geteuid() != 0)
            ? hflow({text("Note: "),
                     text("Some disks won't appear unless you are root user.") |
                         bold | color(Color::Yellow)})
            : emptyElement(),
#endif
        paragraph(
            "Disk capacity must be correctly detected for a successful "
            "recovery.\n"
            "If a disk listed above has an incorrect size, check HD jumper "
            "settings and BIOS "
            "detection, and install the latest OS patches and disk drivers."
        ),
    });
  });

  if (list_disk.empty())
  {
    log_critical("No disk found");
    diskList->Add(Renderer([]() {
      return vbox({
          text("No hard disk found"),
#ifndef DJGPP
          (!isAdmin())
              ?
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
              paragraph("You need to be administrator to use TestDisk++.\n"
                        "select TestDisk++, right-click and choose \"Run as "
                        "administrator\".")
#else
              text("You need to be root to use TestDisk++.")
#endif
              : emptyElement(),
#endif
      });
    }));
  }
  else
  {
    for (auto &disk : list_disk)
    {
      ButtonOption options = ButtonOption::Ascii();
      options.transform    = [disk, &SerialN](const EntryState &s) -> Element {
        if (s.focused)
        {
          if (!disk.serial_no.empty())
            SerialN = hflow({
                text("Serial number: "),
                text(disk.serial_no) | color(Color::Green),
            });
          else
            SerialN = emptyElement();

          return text("> " + s.label) | bgcolor(Color::White) | color(Color::Black) | bold;
        }

        return text("  " + s.label);
      };
      Component diskBtn = Button(
          disk.description_short(disk),
          [disk]() {

          },
          options
      );
      diskList->Add(diskBtn);
    }
  }

  app.Loop(root);
}
