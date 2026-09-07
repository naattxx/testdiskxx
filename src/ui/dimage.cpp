#include "src/dimage.hpp"
#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/node.hpp"
#include "src/common.hpp"
#include "src/intrf.hpp"
#include "src/ui/intrfn.hpp"
#include <cassert>
#include <config.h>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <future>
#include <string_view>

void disk_image_interface(disk_t &disk, const partition_t &partition,
                          const std::filesystem::path &image_dd)
{
  using namespace ftxui;

  if (std::filesystem::exists(image_dd) &&
      !ask_confirmation("Overwrite existing file ? (Y/N)"))
    return;
  float progress                       = 0.f;
  bool stop                            = false;
  std::future<std::string_view> result = std::async([&] -> std::string_view {
    return disk_image(disk, partition, image_dd, &progress, stop);
  });
  auto screen                          = App::Fullscreen();
  auto stopButton                      = Button("  Stop  ", [&] -> void {
    stop = true;
    screen.Exit();
  });
  auto root                            = Renderer(stopButton, [&] -> Element {
    if (progress >= 1.f)
      screen.Exit();
    screen.RequestAnimationFrame();
    return vbox({
        hflow({text("TestDisk++ "), bold(text(VERSION)),
               text(", Data Recovery Utility, "), text(TESTDISKDATE)}),
        text("naattxx"),
        text("https://github.com/naattxx/testdiskxx"),
        separatorEmpty(),
        text(disk.description_short(disk)),
        text(std::format("{:n:>4}",
                         aff_part_aux(AFF_PART_ORDER | AFF_PART_STATUS, disk,
                                      partition))),
        hbox({text(std::format("{:.2f} % ", progress * 100)) | vcenter,
              gauge(progress) | border | flex}),
        separatorEmpty(),
        paragraph(
            "Disk images are mainly used\n"
            "- for forensic purposes\n"
            "- or to deal with media with bad sectors\n\n"
            "To use TestDisk or PhotoRec with this disk image, "
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(_WIN32)
            "open the command line and run\n"
            "   testdisk_win.exe image.dd\n"
            "or photorec_win.exe image.dd"
#else
          "start a Terminal and run\n"
          "   testdisk image.dd\n"
          "or photorec image.dd"
#endif
        ),
        filler(),
        stopButton->Render() | hcenter,
        filler(),
    });
  });
  screen.Loop(root);

  display_message(root, result.get());
}
