#include "texfat.hpp"
#include "src/common.hpp"
#include "src/log.hpp"
#include "src/part/texfat.hpp"
#include "src/ui/askloc.hpp"
#include "src/ui/intrfn.hpp"
#include <config.h>
#include <filesystem>
#include <format>
#include <ftxui/component/app.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/dom/elements.hpp>
#include <utility>

using namespace ftxui;

void exFAT_boot_sector_menu(disk_t &disk, partition_t &partition)
{
  auto screen = App::Fullscreen();
  Component root;

  Element buttonDescription;
  auto buttonOptions = labled_button_options(buttonDescription);

  exfat_boot_sectors boot_sectors = exfat_boot_sectors::scan(disk, partition);

  int selected_button;
  Components buttons {
      Button("[  Quit  ]"
             "Return to Advanced menu",
             screen.ExitLoopClosure(), buttonOptions),
      Maybe(
          Button(
              "[Original BS]"
              "Copy superblock over backup sector",
              [&] -> void {
                if (ask_confirmation(
                        "Copy original exFAT boot record over backup, confirm ?"
                    ))
                {
                  log_info("copy original superblock over backup boot");
                  if (std::cmp_not_equal(
                          disk.pwrite(
                              disk, boot_sectors.original.bootsector.data(),
                              boot_sectors.original.bootsector.size(),
                              partition.part_offset +
                                  boot_sectors.original.bootsector.size()
                          ),
                          boot_sectors.original.bootsector.size()
                      ))
                  {
                    display_message(
                        root,
                        "Write error: Can't overwrite exFAT backup boot record"
                    );
                  }
                  disk.sync(disk);

                  boot_sectors = exfat_boot_sectors::scan(disk, partition);
                }
              },
              buttonOptions
          ),
          [&] -> bool { return boot_sectors.get_options().has_original; }
      ),
      Maybe(Button(
                "[Backup BS]"
                "Copy backup superblock over superblock",
                [&] -> void {
                  if (ask_confirmation(
                          "Copy backup exFAT boot record over main boot "
                          "record, confirm ?"
                      ))
                  {
                    log_info("copy backup superblock over main superblock");

                    /* Reset information about backup boot sector */
                    partition.sb_offset = 0;

                    if (std::cmp_not_equal(
                            disk.pwrite(disk,
                                        boot_sectors.backup.bootsector.data(),
                                        boot_sectors.backup.bootsector.size(),
                                        partition.part_offset),
                            boot_sectors.backup.bootsector.size()
                        ))
                    {
                      display_message(
                          root,
                          "Write error: Can't overwrite exFAT main boot record"
                      );
                    }
                    disk.sync(disk);

                    boot_sectors = exfat_boot_sectors::scan(disk, partition);
                  }
                },
                buttonOptions
            ),
            [&] -> bool { return boot_sectors.get_options().has_backup; }),
      Button(
          "[  Dump  ]"
          "Dump superblock and backup superblock",
          [&] -> void {
            std::filesystem::path path;
            ask_location(path, "Choose where to store the dumps");

            if (path.empty())
              return;

            auto result = boot_sectors.dump(path);
            display_message(root, std::format("superblock        : {}\n"
                                              "backup superblock : {}",
                                              result.first, result.second));
          },
          buttonOptions
      ),
  };

  const auto buttonsContainer =
      Container::Horizontal(buttons, &selected_button) |
      CatchEvent([&](const Event &e) -> bool {
        if (e == Event::q)
        {
          screen.Exit();
          return true;
        }
        return false;
      });

  root = Renderer(buttonsContainer, [&] -> Element {
    return vbox({
        hflow({
            text("TestDisk++ "),
            bold(text(VERSION)),
            text(", Data Recovery Utility, "),
            text(TESTDISKDATE),
        }),
        separatorEmpty(),
        partition_header(disk, partition),
        separatorEmpty(),
        text("Boot record"),
        hflow({text("Status: "), text(boot_sectors.original.status_text())}),
        separatorEmpty(),
        text("Backup boot record"),
        hflow({text("Status: "), text(boot_sectors.backup.status_text())}),
        separatorEmpty(),
        hflow({
            text("Sectors are "),
            boot_sectors.are_identical ? text("identical")
                                       : text("not identical"),
        }),
        filler(),
        buttonsContainer->Render(),
        buttonDescription | hcenter,
    });
  });

  screen.Loop(root);
}
