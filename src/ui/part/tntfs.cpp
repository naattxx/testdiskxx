#include "tntfs.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/dom/elements.hpp"
#include "src/common.hpp"
#include "src/intrf.hpp"
#include "src/io_redir.hpp"
#include "src/log.hpp"
#include "src/part/ntfs_struct.hpp"
#include "src/part/tntfs.hpp"
#include "src/ui/askloc.hpp"
#include "src/ui/dirpart.hpp"
#include "src/ui/intrfn.hpp"
#include <config.h>
#include <filesystem>
#include <format>

using namespace ftxui;

auto ntfs_boot_sector_menu(disk_t &disk, partition_t &partition,
                           const int verbose, const unsigned int expert) -> bool
{
  auto screen = App::Fullscreen();
  Component root;

  auto buttonDescription = text("");
  auto buttonOptions     = ButtonOption();
  buttonOptions.transform =
      [&buttonDescription](const EntryState &s) -> Element {
    if (s.focused)
    {
      buttonDescription = text(s.label.substr(s.label.find(']') + 1));
      return hbox({
          text(">" + s.label.substr(0, s.label.find(']') + 1)) | inverted |
              bold,
          text(" "),
      });
    }

    return text(" " + s.label.substr(0, s.label.find(']') + 1) + " ");
  };

  ntfs_boot_sectors boot_sectors =
      ntfs_boot_sectors::scan(disk, partition, verbose);

  int selected_button;
  auto [has_list, has_original, has_backup, has_mft] =
      boot_sectors.get_options(&selected_button, expert);
  Components buttons {
      Button("[  Quit  ]"
             "Return to Advanced menu",
             screen.ExitLoopClosure(), buttonOptions),

      Maybe(Button(
                "[  List  ]"
                "List directories and files, copy data from NTFS",
                [&] -> void {
                  if (!has_original && has_backup)
                  {
                    io_redir_add_redir(disk, partition.part_offset,
                                       NTFS_BOOT_SECTOR_SIZE, 0,
                                       boot_sectors.backup.bootsector.data());
                    dir_partition(disk, partition, 0, expert);
                    io_redir_del_redir(disk, partition.part_offset);
                  }
                  else
                    dir_partition(disk, partition, 0, expert);
                },
                buttonOptions
            ),
            &has_list),
      Maybe(Button(
                "[Original BS]"
                "Copy boot sector over backup sector",
                [&] -> void {
                  if (ask_confirmation("Copy original NTFS boot sector over "
                                       "backup boot. confirm ?"))
                  {
                    log_info("copy original boot sector over backup boot");
                    if (disk.pwrite(
                            disk, boot_sectors.original.bootsector.data(),
                            NTFS_BOOT_SECTOR_SIZE,
                            partition.part_offset + partition.part_size -
                                disk.sector_size
                        ) != NTFS_BOOT_SECTOR_SIZE)
                    {
                      display_message(
                          root,
                          "Write error: Can't overwrite NTFS backup boot sector"
                      );
                    }
                    disk.sync(disk);
                  }
                },
                buttonOptions
            ),
            &has_original),
      Maybe(Button(
                "[Backup BS]"
                "Copy backup boot sector over boot sector",
                [&] -> void {
                  if (ask_confirmation("Copy backup NTFS boot sector over boot "
                                       "sector, confirm ?"))
                  {
                    log_info("copy backup boot sector over boot sector");

                    /* Reset information about backup boot sector */
                    partition.sb_offset = 0;

                    if (disk.pwrite(disk, boot_sectors.backup.bootsector.data(),
                                    NTFS_BOOT_SECTOR_SIZE,
                                    partition.part_offset) !=
                        NTFS_BOOT_SECTOR_SIZE)
                    {
                      display_message(
                          root, "Write error: Can't overwrite NTFS boot sector"
                      );
                    }
                    disk.sync(disk);
                  }
                },
                buttonOptions
            ),
            &has_backup),
      Button(
          "[Rebuild BS]"
          "Rebuild boot sector",
          [&] -> void {
            // rebuild_NTFS_BS(disk, partition, verbose, expert, current_cmd);
          },
          buttonOptions
      ),
      Maybe(Button(
                "[Repair MFT]"
                "Check MFT",
                [&] -> void {
                  //repair_MFT(disk, partition, verbose, expert, current_cmd);
                },
                buttonOptions
            ),
            &has_mft),
      Button(
          "[  Dump  ]"
          "Dump boot sector and backup boot sector",
          [&] -> void {
            std::filesystem::path path;
            ask_location(path, "Choose where to store the dumps");

            if (path.empty())
              return;

            auto result = boot_sectors.dump(path);
            display_message(root, std::format("bootsector        : {}\n"
                                              "backup bootsector : {}",
                                              result.first, result.second));
          },
          buttonOptions
      ),
  };

  auto buttonsContainer = Container::Horizontal(buttons, &selected_button) |
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
        text(std::format("{:n:>4}",
                         aff_part_aux(AFF_PART::ORDER | AFF_PART::STATUS, disk,
                                      partition))),
        separatorEmpty(),
        text("Boot sector"),
        hflow({text("Status: "), text(boot_sectors.original.status_text())}),
        separatorEmpty(),
        text("Backup boot sector"),
        hflow({text("Status: "), text(boot_sectors.backup.status_text())}),
        separatorEmpty(),
        hflow({
            text("Sectors are "),
            boot_sectors.are_identical ? text("identical")
                                       : text("not identical"),
        }),
        separatorEmpty(),
        paragraph("A valid NTFS Boot sector must be present in order to access "
                  "any data; even if the partition is not bootable."),
        filler(),
        buttonsContainer->Render(),
    });
  });

  screen.Loop(root);
  return true;
}
