#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <list>
#include <string>
#include <config.h>
#include <string_view>

struct [[gnu::gcc_struct,gnu::packed]] efi_guid_t
{
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint8_t clock_seq_hi_and_reserved;
    uint8_t clock_seq_low;
    std::array<uint8_t, 6> node;
};

#define DEFAULT_SECTOR_SIZE 0x200u

/* PARTITION TYPE */
#define P_NO_OS 0x00
#define P_12FAT 0x01
#define P_16FAT 0x04
#define P_EXTENDED 0x05
#define P_16FATBD 0x06
#define P_NTFS 0x07
#define P_HPFS 0x07
#define P_EXFAT 0x07
#define P_OS2MB 0x0A
#define P_32FAT 0x0B
#define P_32FAT_LBA 0x0C
#define P_16FATBD_LBA 0x0E
#define P_EXTENDX 0x0F
#define P_12FATH 0x11
#define P_16FATH 0x14
#define P_16FATBDH 0x16
#define P_NTFSH 0x17
#define P_32FATH 0x1B
#define P_32FAT_LBAH 0x1C
#define P_16FATBD_LBAH 0x1E
#define P_SYSV 0x63
#define P_NETWARE 0x65
#define P_OLDLINUX 0x81
#define P_LINSWAP 0x82
#define P_LINUX 0x83
#define P_LINUXEXTENDX 0x85
#define P_LVM 0x8E
#define P_FREEBSD 0xA5
#define P_OPENBSD 0xA6
#define P_NETBSD 0xA9
#define P_HFS 0xAF
#define P_HFSP 0xAF
#define P_SUN 0xBF
#define P_BEOS 0xEB
#define P_VMFS 0xFB
#define P_RAID 0xFD
#define P_UNK 255
#define NO_ORDER 255
/* Partition SUN */
#define PSUN_BOOT 1
#define PSUN_ROOT 2
#define PSUN_SWAP 3
#define PSUN_USR 4
#define PSUN_WHOLE_DISK 5
#define PSUN_STAND 6
#define PSUN_VAR 7
#define PSUN_HOME 8
#define PSUN_ALT 9
#define PSUN_CACHEFS 10
#define PSUN_LINSWAP P_LINSWAP
#define PSUN_LINUX P_LINUX
#define PSUN_LVM P_LVM
#define PSUN_RAID P_RAID
#define PSUN_UNK 255

#define PHUMAX_PARTITION 1

#define PMAC_DRIVER43 1
#define PMAC_DRIVERATA 2
#define PMAC_DRIVERIO 3
#define PMAC_FREE 4
#define PMAC_FWDRIVER 5
#define PMAC_SWAP 0x82
#define PMAC_LINUX 0x83
#define PMAC_BEOS 0xEB
#define PMAC_HFS 0xAF
#define PMAC_MAP 6
#define PMAC_PATCHES 7
#define PMAC_UNK 8
#define PMAC_NewWorld 9
#define PMAC_DRIVER 10
#define PMAC_MFS 11
#define PMAC_PRODOS 12
#define PMAC_FAT32 13

#define PXBOX_UNK 0
#define PXBOX_FATX 1

#ifdef TESTDISK_LSB
#define be16(x) std::byteswap(x)
#define be24(x) std::byteswap(x)
#define be32(x) std::byteswap(x)
#define be64(x) std::byteswap(x)
#define le16(x) (x) /* x as little endian */
#define le24(x) (x)
#define le32(x) (x)
#define le64(x) (x)
#else /* bigendian */
#define be16(x) (x)
#define be24(x) (x)
#define be32(x) (x)
#define be64(x) (x)
#define le16(x) std::byteswap(x)
#define le24(x) std::byteswap(x)
#define le32(x) std::byteswap(x)
#define le64(x) std::byteswap(x)
#endif

constexpr efi_guid_t GPT_ENT_TYPE_UNUSED
    {
        .time_low=le32(0x00000000), .time_mid=le16(0x0000), .time_hi_and_version=le16(0x0000), .clock_seq_hi_and_reserved=0x00, .clock_seq_low=0x00, .node={0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
    };
constexpr efi_guid_t GPT_ENT_TYPE_EFI
    {
        .time_low=le32(0xc12a7328), .time_mid=le16(0xf81f), .time_hi_and_version=le16(0x11d2), .clock_seq_hi_and_reserved=0xba, .clock_seq_low=0x4b, .node={0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b}
    };
/* Extended Boot Partition */
constexpr efi_guid_t GPT_ENT_TYPE_EBP
    {
        .time_low=le32(0xbc13c2ff), .time_mid=le16(0x59e6), .time_hi_and_version=le16(0x4262), .clock_seq_hi_and_reserved=0xa3, .clock_seq_low=0x52, .node={0xb2, 0x75, 0xfd, 0x6f, 0x71, 0x72}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MBR
    {
        .time_low=le32(0x024dee41), .time_mid=le16(0x33e7), .time_hi_and_version=le16(0x11d3), .clock_seq_hi_and_reserved=0x9d, .clock_seq_low=0x69, .node={0x00, 0x08, 0xc7, 0x81, 0xf3, 0x9f}
    };
constexpr efi_guid_t GPT_ENT_TYPE_FREEBSD
    {
        .time_low=le32(0x516e7cb4), .time_mid=le16(0x6ecf), .time_hi_and_version=le16(0x11d6), .clock_seq_hi_and_reserved=0x8f, .clock_seq_low=0xf8, .node={0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b}
    };
constexpr efi_guid_t GPT_ENT_TYPE_FREEBSD_SWAP
    {
        .time_low=le32(0x516e7cb5), .time_mid=le16(0x6ecf), .time_hi_and_version=le16(0x11d6), .clock_seq_hi_and_reserved=0x8f, .clock_seq_low=0xf8, .node={0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b}
    };
constexpr efi_guid_t GPT_ENT_TYPE_FREEBSD_UFS
    {
        .time_low=le32(0x516e7cb6), .time_mid=le16(0x6ecf), .time_hi_and_version=le16(0x11d6), .clock_seq_hi_and_reserved=0x8f, .clock_seq_low=0xf8, .node={0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b}
    };
constexpr efi_guid_t GPT_ENT_TYPE_FREEBSD_ZFS
    {
        .time_low=le32(0x516e7cb), .time_mid=le16(0x6ecf), .time_hi_and_version=le16(0x11d6), .clock_seq_hi_and_reserved=0x8f, .clock_seq_low=0xf8, .node={0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b}
    };
/*
 * The following is unused but documented here to avoid reuse.
 */
[[maybe_unused, gnu::unused]]
 constexpr efi_guid_t GPT_ENT_TYPE_FREEBSD_UFS2
    {
        .time_low=le32(0x516e7cb7),.time_mid=le16(0x6ecf),.time_hi_and_version=le16(0x11d6),.clock_seq_hi_and_reserved=0x8f,.clock_seq_low=0xf8,.node={0x00,0x02,0x2d,0x09,0x71,0x2b}
    };

constexpr efi_guid_t GPT_ENT_TYPE_FREEBSD_VINUM
    {
        .time_low=le32(0x516e7cb8), .time_mid=le16(0x6ecf), .time_hi_and_version=le16(0x11d6), .clock_seq_hi_and_reserved=0x8f, .clock_seq_low=0xf8, .node={0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b}
    };

constexpr efi_guid_t GPT_ENT_TYPE_MS_BASIC_DATA
    {
        .time_low=le32(0xebd0a0a2), .time_mid=le16(0xb9e5), .time_hi_and_version=le16(0x4433), .clock_seq_hi_and_reserved=0x87, .clock_seq_low=0xc0, .node={0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MS_LDM_DATA
    {
        .time_low=le32(0xaf9b60a0), .time_mid=le16(0x1431), .time_hi_and_version=le16(0x4f62), .clock_seq_hi_and_reserved=0xbc, .clock_seq_low=0x68, .node={0x33, 0x11, 0x71, 0x4a, 0x69, 0xad}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MS_LDM_METADATA
    {
        .time_low=le32(0x5808c8aa), .time_mid=le16(0x7e8f), .time_hi_and_version=le16(0x42e0), .clock_seq_hi_and_reserved=0x85, .clock_seq_low=0xd2, .node={0xe1, 0xe9, 0x04, 0x34, 0xcf, 0xb3}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MS_RECOVERY
    {
        .time_low=le32(0xde94bba4), .time_mid=le16(0x06d1), .time_hi_and_version=le16(0x4d40), .clock_seq_hi_and_reserved=0xa1, .clock_seq_low=0x6a, .node={0xbf, 0xd5, 0x01, 0x79, 0xd6, 0xac}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MS_RESERVED
    {
        .time_low=le32(0xe3c9e316), .time_mid=le16(0x0b5c), .time_hi_and_version=le16(0x4db8), .clock_seq_hi_and_reserved=0x81, .clock_seq_low=0x7d, .node={0xf9, 0x2d, 0xf0, 0x02, 0x15, 0xae}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MS_SPACES
    {
        .time_low=le32(0xe75caf8f), .time_mid=le16(0xf680), .time_hi_and_version=le16(0x4cee), .clock_seq_hi_and_reserved=0xaf, .clock_seq_low=0xa3, .node={0xb0, 0x01, 0xe5, 0x6e, 0xfc, 0x2d}
    };

constexpr efi_guid_t GPT_ENT_TYPE_LINUX_DATA
    {
        .time_low=le32(0x0fc63daf), .time_mid=le16(0x8483), .time_hi_and_version=le16(0x4772), .clock_seq_hi_and_reserved=0x8e, .clock_seq_low=0x79, .node={0x3d, 0x69, 0xd8, 0x47, 0x7d, 0xe4}
    };
constexpr efi_guid_t GPT_ENT_TYPE_LINUX_HOME
    {
        .time_low=le32(0x933ac7e1), .time_mid=le16(0x2eb4), .time_hi_and_version=le16(0x4f13), .clock_seq_hi_and_reserved=0xb8, .clock_seq_low=0x44, .node={0x0e, 0x14, 0xe2, 0xae, 0xf9, 0x15}
    };
constexpr efi_guid_t GPT_ENT_TYPE_LINUX_LVM
    {
        .time_low=le32(0xe6d6d379), .time_mid=le16(0xf507), .time_hi_and_version=le16(0x44c2), .clock_seq_hi_and_reserved=0xa2, .clock_seq_low=0x3c, .node={0x23, 0x8f, 0x2a, 0x3d, 0xf9, 0x28}
    };
constexpr efi_guid_t GPT_ENT_TYPE_LINUX_RAID
    {
        .time_low=le32(0xa19d880f), .time_mid=le16(0x05fc), .time_hi_and_version=le16(0x4d3b), .clock_seq_hi_and_reserved=0xa0, .clock_seq_low=0x06, .node={0x74, 0x3f, 0x0f, 0x84, 0x91, 0x1e}
    };
constexpr efi_guid_t GPT_ENT_TYPE_LINUX_RESERVED
    {
        .time_low=le32(0x8da63339), .time_mid=le16(0x0007), .time_hi_and_version=le16(0x60c0), .clock_seq_hi_and_reserved=0xc4, .clock_seq_low=0x36, .node={0x08, 0x3a, 0xc8, 0x23, 0x09, 0x08}
    };
constexpr efi_guid_t GPT_ENT_TYPE_LINUX_SRV
    {
        .time_low=le32(0x3b8f8425), .time_mid=le16(0x20e0), .time_hi_and_version=le16(0x4f3b), .clock_seq_hi_and_reserved=0x90, .clock_seq_low=0x7f, .node={0x1a, 0x25, 0xa7, 0x6f, 0x98, 0xe8}
    };
constexpr efi_guid_t GPT_ENT_TYPE_LINUX_SWAP
    {
        .time_low=le32(0x0657fd6d), .time_mid=le16(0xa4ab), .time_hi_and_version=le16(0x43c4), .clock_seq_hi_and_reserved=0x84, .clock_seq_low=0xe5, .node={0x09, 0x33, 0xc8, 0x4b, 0x4f, 0x4f}
    };

constexpr efi_guid_t GPT_ENT_TYPE_HPUX_DATA
    {
        .time_low=le32(0x75894c1e), .time_mid=le16(0x3aeb), .time_hi_and_version=le16(0x11d3), .clock_seq_hi_and_reserved=0xb7, .clock_seq_low=0xc1, .node={0x7b, 0x03, 0xa0, 0x00, 0x00, 0x00}
    };
constexpr efi_guid_t GPT_ENT_TYPE_HPUX_SERVICE
    {
        .time_low=le32(0xe2a1e728), .time_mid=le16(0x32e3), .time_hi_and_version=le16(0x11d6), .clock_seq_hi_and_reserved=0xa6, .clock_seq_low=0x82, .node={0x7b, 0x03, 0xa0, 0x00, 0x00, 0x00}
    };

constexpr efi_guid_t GPT_ENT_TYPE_APPLE_CORE_STORAGE
    {
        .time_low=le32(0x53746F72), .time_mid=le16(0x6167), .time_hi_and_version=le16(0x11aa), .clock_seq_hi_and_reserved=0xaa, .clock_seq_low=0x11, .node={0x00, 0x30, 0x65, 0x43, 0xec, 0xac}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MAC_APFS
    {
        .time_low=le32(0x7c3457ef), .time_mid=le16(0x0000), .time_hi_and_version=le16(0x11aa), .clock_seq_hi_and_reserved=0xaa, .clock_seq_low=0x11, .node={0x00, 0x30, 0x65, 0x43, 0xec, 0xac}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MAC_BOOT
    {
        .time_low=le32(0x426f6f74), .time_mid=le16(0x0000), .time_hi_and_version=le16(0x11aa), .clock_seq_hi_and_reserved=0xaa, .clock_seq_low=0x11, .node={0x00, 0x30, 0x65, 0x43, 0xec, 0xac}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MAC_HFS
    {
        .time_low=le32(0x48465300), .time_mid=le16(0x0000), .time_hi_and_version=le16(0x11aa), .clock_seq_hi_and_reserved=0xaa, .clock_seq_low=0x11, .node={0x00, 0x30, 0x65, 0x43, 0xec, 0xac}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MAC_LABEL
    {
        .time_low=le32(0x4c616265), .time_mid=le16(0x6c00), .time_hi_and_version=le16(0x11aa), .clock_seq_hi_and_reserved=0xaa, .clock_seq_low=0x11, .node={0x00, 0x30, 0x65, 0x43, 0xec, 0xac}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MAC_RAID
    {
        .time_low=le32(0x52414944), .time_mid=le16(0x0000), .time_hi_and_version=le16(0x11aa), .clock_seq_hi_and_reserved=0xaa, .clock_seq_low=0x11, .node={0x00, 0x30, 0x65, 0x43, 0xec, 0xac}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MAC_RAID_OFFLINE
    {
        .time_low=le32(0x52414944), .time_mid=le16(0x5f4f), .time_hi_and_version=le16(0x11aa), .clock_seq_hi_and_reserved=0xaa, .clock_seq_low=0x11, .node={0x00, 0x30, 0x65, 0x43, 0xec, 0xac}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MAC_TV_RECOVERY
    {
        .time_low=le32(0x5265636f), .time_mid=le16(0x7665), .time_hi_and_version=le16(0x11aa), .clock_seq_hi_and_reserved=0xaa, .clock_seq_low=0x11, .node={0x00, 0x30, 0x65, 0x43, 0xec, 0xac}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MAC_UFS
    {
        .time_low=le32(0x55465300), .time_mid=le16(0x0000), .time_hi_and_version=le16(0x11aa), .clock_seq_hi_and_reserved=0xaa, .clock_seq_low=0x11, .node={0x00, 0x30, 0x65, 0x43, 0xec, 0xac}
    };

constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_BACKUP
    {
        .time_low=le32(0x6a8b642b), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x99, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_BOOT
    {
        .time_low=le32(0x6a82cb45), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x99, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_ROOT
    {
        .time_low=le32(0x6a85cf4d), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x99, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_SWAP
    {
        .time_low=le32(0x6a87c46f), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x99, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_USR
    {
        .time_low=le32(0x6a898cc3), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x99, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_MAC_ZFS = GPT_ENT_TYPE_SOLARIS_USR;
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_VAR
    {
        .time_low=le32(0x6a8ef2e9), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x99, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_HOME
    {
        .time_low=le32(0x6a90ba39), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x96, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_EFI_ALTSCTR
    {
        .time_low=le32(0x6a9283a5), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x96, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_RESERVED1
    {
        .time_low=le32(0x6a945a3b), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x96, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_RESERVED2
    {
        .time_low=le32(0x6a9630d1), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x96, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_RESERVED3
    {
        .time_low=le32(0x6a980767), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x96, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_RESERVED4
    {
        .time_low=le32(0x6a96237f), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x96, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };
constexpr efi_guid_t GPT_ENT_TYPE_SOLARIS_RESERVED5
    {
        .time_low=le32(0x6a8d2ac7), .time_mid=le16(0x1dd2), .time_hi_and_version=le16(0x11b2), .clock_seq_hi_and_reserved=0x96, .clock_seq_low=0xa6, .node={0x08, 0x00, 0x20, 0x73, 0x66, 0x31}
    };

constexpr efi_guid_t GPT_ENT_TYPE_BEOS_BFS
    {
        .time_low=le32(0x42465331), .time_mid=le16(0x3ba3), .time_hi_and_version=le16(0x10f1), .clock_seq_hi_and_reserved=0x80, .clock_seq_low=0x2a, .node={0x48, 0x61, 0x69, 0x6b, 0x75, 0x21}
    };

#define TESTDISK_O_RDONLY 00
#define TESTDISK_O_RDWR 02
#define TESTDISK_O_DIRECT 040000
#define TESTDISK_O_READAHEAD_8K 04
#define TESTDISK_O_READAHEAD_32K 010
#define TESTDISK_O_ALL 020

enum upart_type_t
{
    UP_UNK = 0,
    UP_APFS,
    UP_BEOS,
    UP_BTRFS,
    UP_CRAMFS,
    UP_EXFAT,
    UP_EXT2,
    UP_EXT3,
    UP_EXT4,
    UP_EXTENDED,
    UP_FAT12,
    UP_FAT16,
    UP_FAT32,
    UP_FATX,
    UP_FREEBSD,
    UP_F2FS,
    UP_GFS2,
    UP_HFS,
    UP_HFSP,
    UP_HFSX,
    UP_HPFS,
    UP_ISO,
    UP_JFS,
    UP_LINSWAP,
    UP_LINSWAP2,
    UP_LINSWAP_8K,
    UP_LINSWAP2_8K,
    UP_LINSWAP2_8KBE,
    UP_LUKS,
    UP_LVM,
    UP_LVM2,
    UP_MD,
    UP_MD1,
    UP_NETWARE,
    UP_NTFS,
    UP_OPENBSD,
    UP_OS2MB,
    UP_ReFS,
    UP_RFS,
    UP_RFS2,
    UP_RFS3,
    UP_RFS4,
    UP_SUN,
    UP_SYSV4,
    UP_UFS,
    UP_UFS2,
    UP_UFS_LE,
    UP_UFS2_LE,
    UP_VMFS,
    UP_WBFS,
    UP_XFS,
    UP_XFS2,
    UP_XFS3,
    UP_XFS4,
    UP_XFS5,
    UP_ZFS
};
enum status_type_t
{
    STATUS_DELETED,
    STATUS_PRIM,
    STATUS_PRIM_BOOT,
    STATUS_LOG,
    STATUS_EXT,
    STATUS_EXT_IN_EXT
};
enum errcode_type_t
{
    BAD_NOERR,
    BAD_SS,
    BAD_ES,
    BAD_SH,
    BAD_EH,
    BAD_EBS,
    BAD_RS,
    BAD_SC,
    BAD_EC,
    BAD_SCOUNT
};

#define AFF_PART_BASE 0
#define AFF_PART_ORDER 1
#define AFF_PART_STATUS 2

enum class UNIT
{
    DEFAULT = 0,
    SECTOR,
    CHS
};

struct disk_t;
struct partition_t;
/*@
    predicate valid_partition(partition_t &part) = (\valid_read(part));
  @*/

struct CHSgeometry_t
{
    unsigned long int cylinders;
    unsigned int heads_per_cylinder;
    unsigned int sectors_per_head;
    unsigned int bytes_per_sector; /* WARN: may be uninitialized */
};

struct CHS_t
{
    unsigned long int cylinder;
    unsigned int head;
    unsigned int sector;
};

using list_part_t = std::list<partition_t>;

/*@
inductive valid_list_part{L} (list_part_t *list)
{
  case list_null{L}:
    valid_list_part(\null);
  case list_not_null{L}:
    \forall list_part_t *list; \valid_read(list) ==> valid_list_part(list->next) ==> valid_list_part(list);
}
  @*/

using list_disk_t = std::list<disk_t>;

/*@
inductive ld_reachable{L} (list_disk_t* root, list_disk_t* node)
{
  case root_ld_reachable{L}:
    \forall list_disk_t *root; ld_reachable(root,root);
  case next_ld_reachable{L}:
    \forall list_disk_t *root, *node; \valid(root) ==> ld_reachable(root->next, node) ==> ld_reachable(root,node);
}
*/

/*@ predicate ld_finite{L}(list_disk_t* root) = ld_reachable(root,\null); */

struct systypes
{
    const unsigned int part_type;
    const char *name;
};

struct arch_fnct_t
{
    const char *part_name;
    const char *part_name_option;
    const char *msg_part_type;
    list_part_t (*read_part)(disk_t &disk, const int verbose, const int saveheader);
    int (*write_part)(disk_t &disk, const list_part_t &list_part, const int ro, const int verbose);
    void (*init_part_order)(const disk_t &disk, list_part_t &list_part);
    /* geometry must be initialized to 0,0,0 in get_geometry_from_mbr()*/
    int (*get_geometry_from_mbr)(const unsigned char *buffer, const int verbose, CHSgeometry_t *geometry);
    int (*check_part)(disk_t &disk, const int verbose, partition_t &partition, const int saveheader);
    int (*write_MBR_code)(disk_t &disk);
    void (*set_prev_status)(const disk_t &disk, partition_t &partition);
    void (*set_next_status)(const disk_t &disk, partition_t &partition);
    int (*test_structure)(const list_part_t &list_part);
    unsigned int (*get_part_type)(const partition_t &partition);
    int (*set_part_type)(partition_t &partition, unsigned int part_type);
    void (*init_structure)(const disk_t &disk, list_part_t &list_part, const int verbose);
    int (*erase_list_part)(disk_t &disk);
    const char *(*get_partition_typename)(const partition_t &partition);
    int (*is_part_known)(const partition_t &partition);
};

/*@
    predicate valid_arch(arch_fnct_t *arch) = (
      \valid_read(arch) &&
      (arch->get_geometry_from_mbr==\null || \valid_function(arch->get_geometry_from_mbr))
    );
  @*/

struct disk_t
{
    std::string description_txt;
    std::string description_short_txt;
    CHSgeometry_t geom; /* logical CHS */
    uint64_t disk_size{0};
    std::string device;
    std::string model;
    std::string serial_no;
    std::string fw_rev;
    disk_t();
    ~disk_t();
    void update_fields();
    void update_geometry(const int verbose);
    void autoset_geometry(const unsigned char *buffer, const int verbose);
    void autodetect_arch(const arch_fnct_t *arch);
    void autoset_unit();
    [[nodiscard]]
    auto is_hpa_or_dco() const -> int;
    std::string_view(*description)(disk_t &disk);
    std::string_view(*description_short)(disk_t &disk);
    int (*pread)(disk_t &disk, void *buf, const unsigned int count, const uint64_t offset);
    int (*pwrite)(disk_t &disk, const void *buf, const unsigned int count, const uint64_t offset);
    int (*sync)(disk_t &disk);
    void (*clean)(disk_t &disk);
    const arch_fnct_t *arch;
    const arch_fnct_t *arch_autodetected;
    void *data;
    uint64_t disk_real_size;
    uint64_t user_max{0};
    uint64_t native_max{0};
    uint64_t dco{0};
    uint64_t offset{0}; /* offset to first sector, may be modified in the futur to handle broken raid */
    char *rbuffer{nullptr};
    char *wbuffer{nullptr};
    unsigned int rbuffer_size{0};
    unsigned int wbuffer_size{0};
    int write_used{0};
    int autodetect{0};
    int access_mode;
    UNIT unit{UNIT::CHS};
    unsigned int sector_size;
};

/*@
    predicate valid_disk(disk_t *disk) =
      (\valid_read(disk) &&
       \freeable(disk) &&
       valid_read_string(disk->device) &&
       \freeable(disk->device) &&
       \valid_function(disk->clean) &&
       \valid_function(disk->description) &&
       (disk->model == \null || \freeable(disk->model)) &&
       (disk->model == \null || valid_read_string(disk->model)) &&
       (disk->serial_no == \null || \freeable(disk->serial_no)) &&
       (disk->fw_rev == \null || \freeable(disk->fw_rev)) &&
       (disk->data == \null || \freeable(disk->data)) &&
       (disk->rbuffer == \null || (\freeable(disk->rbuffer) && disk->rbuffer_size > 0)) &&
       (disk->wbuffer == \null || (\freeable(disk->wbuffer) && disk->wbuffer_size > 0)) &&
       valid_arch(disk->arch) &&
       disk->sector_size > 0
      );
*/

/*@
inductive valid_list_disk{L} (list_disk_t *list)
{
  case list_null{L}:
    valid_list_disk(\null);
  case list_not_null{L}:
    \forall list_disk_t *list; \valid_read(list) && valid_list_disk(list) ==> valid_disk(list->disk) &&
valid_list_disk(list->next);
}
  @*/

struct partition_t
{
    int to_be_removed;

    void set_name(std::string_view src);
    void set_name_chomp(std::string_view src);
    void reset(const arch_fnct_t *arch);
    partition_t() = default;
    partition_t(const arch_fnct_t *arch);

    std::string fsname;
    std::string partname;
    std::string info;
    uint64_t part_offset;
    uint64_t part_size;
    uint64_t sborg_offset;
    uint64_t sb_offset;
    unsigned int sb_size;
    unsigned int blocksize;
    efi_guid_t part_uuid;
    efi_guid_t part_type_gpt;
    unsigned int part_type_humax;
    unsigned int part_type_i386;
    unsigned int part_type_mac;
    unsigned int part_type_sun;
    unsigned int part_type_xbox;
    upart_type_t upart_type;
    status_type_t status;
    unsigned int order;
    errcode_type_t errcode;
    const arch_fnct_t *arch;
    /* NTFS => utils_cluster_in_use */
    /* ext2/ext3/ext4 */
#if 0
  int (*is_allocated)(disk_t &disk, const partition_t &partition, const uint64_t offset);
  void *free_is_allocated(void);
#endif
};

struct my_data_t
{
    disk_t *disk_car;
    partition_t partition;
    uint64_t offset;
};

/*@
  @ requires valid_read_string(str);
  @ ensures \result == \null || valid_read_string(\result);
  @*/
auto strip_dup(char *str) -> char *;

/*@
  @ requires f_time <= 0xffffffff;
  @ requires f_date <= 0xffffffff;
  @ terminates \true;
  @ assigns \nothing;
  @*/
auto date_dos2unix(const unsigned short f_time, const unsigned short f_date) -> time_t;

void set_secwest();

/*@
  @ terminates \true;
  @ assigns \nothing;
  @*/
auto td_ntfs2utc(int64_t ntfstime) -> time_t;

#ifndef BSD_MAXPARTITIONS
#define BSD_MAXPARTITIONS 8
#endif
#ifndef OPENBSD_MAXPARTITIONS
#define OPENBSD_MAXPARTITIONS 16
#endif
#if !defined(HAVE_LOCALTIME_R) && !defined(__MINGW32__) && !defined(DISABLED_FOR_FRAMAC)
/*@
  @ requires valid_timer: \valid_read(timep);
  @ requires \valid(result);
  @*/
auto localtime_r(const time_t *timep, struct tm *result) -> struct tm *;
#endif

/*@
  @ requires \valid(current_cmd);
  @ requires valid_read_string(*current_cmd);
  @ requires valid_read_string(cmd);
  @ requires \separated(cmd+(..), current_cmd, *current_cmd);
  @ requires strlen(cmd) == n;
  @ assigns  *current_cmd;
  @ ensures  valid_read_string(*current_cmd);
  @ ensures  \result != 0 ==> *current_cmd == \old(*current_cmd);
  @*/
// ensures  \result == 0 ==> *current_cmd == \old(*current_cmd) + n;
// assigns \result \from indirect:(*current_cmd)[0 .. n-1], indirect:cmd[0 ..n-1], indirect:n;
auto check_command(char **current_cmd, const char *cmd, const size_t n) -> int;

/*@
  @ requires \valid(current_cmd);
  @ requires valid_read_string(*current_cmd);
  @ requires \separated(current_cmd, *current_cmd);
  @ terminates \true;
  @ assigns  *current_cmd;
  @ ensures  valid_read_string(*current_cmd);
  @*/
void skip_comma_in_command(char **current_cmd);

/*@
  @ requires \valid(current_cmd);
  @ requires valid_read_string(*current_cmd);
  @ requires \separated(current_cmd, *current_cmd);
  @ terminates \true;
  @ assigns  *current_cmd;
  @ ensures  valid_read_string(*current_cmd);
  @*/
auto get_int_from_command(char **current_cmd) -> uint64_t;
