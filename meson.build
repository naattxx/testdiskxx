project('testdisk++', 'cpp', version: '0.1.0-WIP', meson_version: '>=1.3.0')

compiler = meson.get_compiler('cpp')

conf_data = configuration_data()
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set_quoted('TESTDISKDATE', 'July 2026')
conf_data.set_quoted('TARGET', host_machine.system())
conf_data.set_quoted('COMPILER_ID', compiler.get_id() + ' ' + compiler.version())

libewf = compiler.find_library('libewf', required: false)
libntfs = compiler.find_library('libntfs', required: false)
libntfs3g = compiler.find_library('libntfs-3g', required: false)
libext2fs = compiler.find_library('libext2fs', required: false)
reiserfs = compiler.find_library('reiserfs', required: false)
libiconv = compiler.find_library('libiconv', required: false)
conf_data.set('HAVE_LIBEWF', libewf.found())
conf_data.set('HAVE_LIBNTFS', libntfs.found())
conf_data.set('HAVE_LIBNTFS3G', libntfs3g.found())
conf_data.set('HAVE_LIBEXT2FS', libext2fs.found())
conf_data.set('HAVE_LIBREISERFS', reiserfs.found())
conf_data.set('HAVE_ICONV', libiconv.found())

conf_data.set(
  'NTFS_MBSTOUCS_HAVE_TWO_ARGUMENTS',
  compiler.compiles(
    '''
  #define HAVE_SYS_TYPES_H
  #define HAVE_SYS_STAT_H
  #define HAVE_TIME_H
  #include <cstdarg>
  extern "C" {
  #if __has_include(<ntfs/volume.h>)
  #include <ntfs/volume.h>
  #include <ntfs/attrib.h>
  #elif __has_include(<ntfs-3g/volume.h>)
  #include <ntfs-3g/volume.h>
  #include <ntfs-3g/attrib.h>
  #endif
  void test(){
  ntfs_mbstoucs(NULL,NULL);
  }
  }
  ''',
    name: 'NTFS_MBSTOUCS_HAVE_TWO_ARGUMENTS',
  ),
)

conf_data.set(
  'NTFS_DEVICE_OPERATIONS_IOCTL_INT',
  compiler.compiles(
    '''
  #define HAVE_SYS_TYPES_H
  #define HAVE_SYS_STAT_H
  #define HAVE_TIME_H
  #include <cstdarg>
  extern "C" {
  #if __has_include(<ntfs/device.h>)
  #include <ntfs/device.h>
  #elif __has_include(<ntfs-3g/device.h>)
  #include <ntfs-3g/types.h>
  #include <ntfs-3g/device.h>
  #endif
  static int ntfs_device_testdisk_io_ioctl(struct ntfs_device *dev, int request, void *argp)
  {
    return -1;
  }
  struct ntfs_device_operations ntfs_device_testdisk_io_ops = {
      .ioctl = &ntfs_device_testdisk_io_ioctl,
  };
  }
  ''',
    name: 'NTFS_DEVICE_OPERATIONS_IOCTL_INT',
    werror: true,
  ),
)

conf_data.set(
  'NTFS_DEVICE_OPERATIONS_IOCTL_UL',
  compiler.compiles(
    '''
  #define HAVE_SYS_TYPES_H
  #define HAVE_SYS_STAT_H
  #define HAVE_TIME_H
  #include <cstdarg>
  extern "C" {
  #if __has_include(<ntfs/device.h>)
  #include <ntfs/device.h>
  #elif __has_include(<ntfs-3g/device.h>)
  #include <ntfs-3g/types.h>
  #include <ntfs-3g/device.h>
  #endif
  static int ntfs_device_testdisk_io_ioctl(struct ntfs_device *dev, unsigned long request, void *argp)
  {
    return -1;
  }
  struct ntfs_device_operations ntfs_device_testdisk_io_ops = {
      .ioctl = &ntfs_device_testdisk_io_ioctl,
  };
  }
  ''',
    name: 'NTFS_DEVICE_OPERATIONS_IOCTL_UL',
    werror: true,
  ),
)

configure_file(output: 'config.h', configuration: conf_data)

spdlog_dep = dependency(
  'spdlog',
  version: '1.17.0',
  fallback: ['spdlog', 'spdlog_dep'],
  required: true,
)

inc_dirs = include_directories('include')
sources = files(
  'src/addpart.cpp',
  'src/adv.cpp',
  'src/analyse.cpp',
  'src/askloc.cpp',
  'src/autoset.cpp',
  'src/chgtype.cpp',
  'src/common.cpp',
  'src/crc.cpp',
  'src/dimage.cpp',
  'src/dir.cpp',
  'src/dirpart.cpp',
  'src/fnctdsk.cpp',
  'src/hdaccess.cpp',
  'src/hdcache.cpp',
  'src/hidden.cpp',
  'src/hpa_doc.cpp',
  'src/intrf.cpp',
  'src/intrface.cpp',
  'src/io_redir.cpp',
  'src/log.cpp',
  'src/log_part.cpp',
  'src/part/apfs.cpp',
  'src/part/apfs_common.cpp',
  'src/part/bfs.cpp',
  'src/part/bsd.cpp',
  'src/part/btrfs.cpp',
  'src/part/cramfs.cpp',
  'src/part/exfat.cpp',
  'src/part/exfat_dir.cpp',
  'src/part/ext2.cpp',
  'src/part/ext2_common.cpp',
  'src/part/ext2_dir.cpp',
  'src/part/ext2_sb.cpp',
  'src/part/ext2_sbn.cpp',
  'src/part/f2fs.cpp',
  'src/part/fat.cpp',
  'src/part/fat1x.cpp',
  'src/part/fat32.cpp',
  'src/part/fat_adv.cpp',
  'src/part/fat_cluster.cpp',
  'src/part/fat_common.cpp',
  'src/part/fat_dir.cpp',
  'src/part/fatx.cpp',
  'src/part/gfs.cpp',
  'src/part/hfs.cpp',
  'src/part/hfsp.cpp',
  'src/part/hpfs.cpp',
  'src/part/iso.cpp',
  'src/part/jfs.cpp',
  'src/part/luks.cpp',
  'src/part/lvm.cpp',
  'src/part/md.cpp',
  'src/part/netware.cpp',
  'src/part/ntfs.cpp',
  'src/part/ntfs_adv.cpp',
  'src/part/ntfs_dir.cpp',
  'src/part/ntfs_fix.cpp',
  'src/part/ntfs_io.cpp',
  'src/part/ntfs_udl.cpp',
  'src/part/ntfs_utl.cpp',
  'src/part/partgpt.cpp',
  'src/part/partgptw.cpp',
  'src/part/parthumax.cpp',
  'src/part/parti386.cpp',
  'src/part/partmac.cpp',
  'src/part/partnone.cpp',
  'src/part/partsun.cpp',
  'src/part/partxbox.cpp',
  'src/part/refs.cpp',
  'src/part/rfs.cpp',
  'src/part/rfs_dir.cpp',
  'src/part/sun.cpp',
  'src/part/swap.cpp',
  'src/part/sysv.cpp',
  'src/part/texfat.cpp',
  'src/part/thfs.cpp',
  'src/part/tntfs.cpp',
  'src/part/ufs.cpp',
  'src/part/vmfs.cpp',
  'src/part/wbfs.cpp',
  'src/part/xfs.cpp',
  'src/part/zfs.cpp',
  'src/partauto.cpp',
  'src/savehdr.cpp',
  'src/setdate.cpp',
  'src/testdisk.cpp',
  'src/ui/tdisksel.cpp',
  'src/ui/tlog.cpp',
  'src/unicode.cpp',
  'src/utils.cpp',
)

deps = [spdlog_dep, libntfs, libntfs3g, libext2fs, libewf, reiserfs, libiconv]

executable(
  'testdisk',
  sources,
  include_directories: inc_dirs,
  dependencies: deps,
)
