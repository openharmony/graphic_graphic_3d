/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_LUME_ASSET_COMPILER_H
#define OHOS_LUME_ASSET_COMPILER_H
enum : uint32_t {
    LC_SEGMENT_64 = 0x19,
    LC_SYMTAB = 0x2,
};

enum : uint8_t {
    N_UNDF = 0x0u,
    N_ABS = 0x2u,
    N_SECT = 0xeu,
    N_PBUD = 0xcu,
    N_INDR = 0xau
};

enum : uint32_t {
    MH_OBJECT = 0x1u,
    MH_EXECUTE = 0x2u,
    MH_FVMLIB = 0x3u,
    MH_CORE = 0x4u,
    MH_PRELOAD = 0x5u,
    MH_DYLIB = 0x6u,
    MH_DYLINKER = 0x7u,
    MH_BUNDLE = 0x8u,
    MH_DYLIB_STUB = 0x9u,
    MH_DSYM = 0xAu,
    MH_KEXT_BUNDLE = 0xBu,
    MH_FILESET = 0xCu,
};

enum : uint32_t {
    CPU_ARCH_MASK = 0xff000000,
    CPU_ARCH_ABI64 = 0x01000000,
    CPU_ARCH_ABI64_32 = 0x02000000,
};

enum : uint32_t {
    CPU_TYPE_ANY = static_cast<uint32_t>(-1),
    CPU_TYPE_X86 = 7,
    CPU_TYPE_I386 = CPU_TYPE_X86,
    CPU_TYPE_X86_64 = CPU_TYPE_X86 | CPU_ARCH_ABI64,
    /* CPU_TYPE_MIPS      = 8, */
    CPU_TYPE_MC98000 = 10,
    CPU_TYPE_ARM = 12,
    CPU_TYPE_ARM64 = CPU_TYPE_ARM | CPU_ARCH_ABI64,
    CPU_TYPE_ARM64_32 = CPU_TYPE_ARM | CPU_ARCH_ABI64_32,
    CPU_TYPE_SPARC = 14,
    CPU_TYPE_POWERPC = 18,
    CPU_TYPE_POWERPC64 = CPU_TYPE_POWERPC | CPU_ARCH_ABI64
};

enum : uint32_t {
    CPU_SUBTYPE_MASK = 0xff000000,
    CPU_SUBTYPE_LIB64 = 0x80000000,
    CPU_SUBTYPE_MULTIPLE = ~0u
};

enum : uint32_t {
    CPU_SUBTYPE_I386_ALL = 3,
    CPU_SUBTYPE_386 = 3,
    CPU_SUBTYPE_486 = 4,
    CPU_SUBTYPE_486SX = 0x84,
    CPU_SUBTYPE_586 = 5,
    CPU_SUBTYPE_PENT = CPU_SUBTYPE_586,
    CPU_SUBTYPE_PENTPRO = 0x16,
    CPU_SUBTYPE_PENTII_M3 = 0x36,
    CPU_SUBTYPE_PENTII_M5 = 0x56,
    CPU_SUBTYPE_CELERON = 0x67,
    CPU_SUBTYPE_CELERON_MOBILE = 0x77,
    CPU_SUBTYPE_PENTIUM_3 = 0x08,
    CPU_SUBTYPE_PENTIUM_3_M = 0x18,
    CPU_SUBTYPE_PENTIUM_3_XEON = 0x28,
    CPU_SUBTYPE_PENTIUM_M = 0x09,
    CPU_SUBTYPE_PENTIUM_4 = 0x0a,
    CPU_SUBTYPE_PENTIUM_4_M = 0x1a,
    CPU_SUBTYPE_ITANIUM = 0x0b,
    CPU_SUBTYPE_ITANIUM_2 = 0x1b,
    CPU_SUBTYPE_XEON = 0x0c,
    CPU_SUBTYPE_XEON_MP = 0x1c,

    CPU_SUBTYPE_X86_ALL = 3,
    CPU_SUBTYPE_X86_64_ALL = 3,
    CPU_SUBTYPE_X86_ARCH1 = 4,
    CPU_SUBTYPE_X86_64_H = 8
};

enum : uint32_t {
    CPU_SUBTYPE_ARM64_ALL = 0,
    CPU_SUBTYPE_ARM64_V8 = 1,
    CPU_SUBTYPE_ARM64E = 2,
};

enum : uint32_t {
    MH_MAGIC = 0xFEEDFACEu,
    MH_CIGAM = 0xCEFAEDFEu,
    MH_MAGIC_64 = 0xFEEDFACFu,
    MH_CIGAM_64 = 0xCFFAEDFEu,
    FAT_MAGIC = 0xCAFEBABEu,
    FAT_CIGAM = 0xBEBAFECAu,
    FAT_MAGIC_64 = 0xCAFEBABFu,
    FAT_CIGAM_64 = 0xBFBAFECAu
};

enum {
    MH_NOUNDEFS = 0x00000001u,
    MH_INCRLINK = 0x00000002u,
    MH_DYLDLINK = 0x00000004u,
    MH_BINDATLOAD = 0x00000008u,
    MH_PREBOUND = 0x00000010u,
    MH_SPLIT_SEGS = 0x00000020u,
    MH_LAZY_INIT = 0x00000040u,
    MH_TWOLEVEL = 0x00000080u,
    MH_FORCE_FLAT = 0x00000100u,
    MH_NOMULTIDEFS = 0x00000200u,
    MH_NOFIXPREBINDING = 0x00000400u,
    MH_PREBINDABLE = 0x00000800u,
    MH_ALLMODSBOUND = 0x00001000u,
    MH_SUBSECTIONS_VIA_SYMBOLS = 0x00002000u,
    MH_CANONICAL = 0x00004000u,
    MH_WEAK_DEFINES = 0x00008000u,
    MH_BINDS_TO_WEAK = 0x00010000u,
    MH_ALLOW_STACK_EXECUTION = 0x00020000u,
    MH_ROOT_SAFE = 0x00040000u,
    MH_SETUID_SAFE = 0x00080000u,
    MH_NO_REEXPORTED_DYLIBS = 0x00100000u,
    MH_PIE = 0x00200000u,
    MH_DEAD_STRIPPABLE_DYLIB = 0x00400000u,
    MH_HAS_TLV_DESCRIPTORS = 0x00800000u,
    MH_NO_HEAP_EXECUTION = 0x01000000u,
    MH_APP_EXTENSION_SAFE = 0x02000000u,
    MH_NLIST_OUTOFSYNC_WITH_DYLDINFO = 0x04000000u,
    MH_SIM_SUPPORT = 0x08000000u,
    MH_DYLIB_IN_CACHE = 0x80000000u
};

enum {
    N_STAB = 0xe0,
    N_PEXT = 0x10,
    N_TYPE = 0x0e,
    N_EXT = 0x01
};

enum { VM_PROT_READ = 0x1, VM_PROT_WRITE = 0x2, VM_PROT_EXECUTE = 0x4 };


enum cpu_subtype_t {};
enum cpu_type_t {};
typedef int vm_prot_t;

struct fat_header {
    uint32_t magic;
    uint32_t nfat_arch;
};

struct fat_arch {
    cpu_type_t cputype;
    cpu_subtype_t cpusubtype;
    uint32_t offset;
    uint32_t size;
    uint32_t align;
};

struct mach_header_64 {
    uint32_t magic;

    cpu_type_t cputype;

    cpu_subtype_t cpusubtype;

    uint32_t filetype;

    uint32_t ncmds;

    uint32_t sizeofcmds;

    uint32_t flags;

    uint32_t reserved;
};

struct symtab_command {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;
    uint32_t symoff;
};

struct section_64 {
    char sectname[16];

    char segname[16];

    uint64_t addr;

    uint64_t size;

    uint32_t offset;

    uint32_t align;

    uint32_t reloff;

    uint32_t nreloc;

    uint32_t flags;

    uint32_t reserved1;

    uint32_t reserved2;

    uint32_t reserved3;
};

struct segment_command_64 {
    uint32_t cmd;

    uint32_t cmdsize;

    char segname[16];

    uint64_t vmaddr;

    uint64_t vmsize;

    uint64_t fileoff;

    uint64_t filesize;

    uint32_t maxprot;

    uint32_t initprot;

    uint32_t nsects;

    uint32_t flags;
};

struct nlist_64 {
    uint32_t n_strx;
    uint8_t n_type;
    uint8_t n_sect;
    uint16_t n_desc;
    uint64_t n_value;
};

enum SectionType : uint32_t {
    S_REGULAR = 0x00u,
    S_ZEROFILL = 0x01u,
    S_CSTRING_LITERALS = 0x02u,
    S_4BYTE_LITERALS = 0x03u,
    S_8BYTE_LITERALS = 0x04u,
    S_LITERAL_POINTERS = 0x05u,
    S_NON_LAZY_SYMBOL_POINTERS = 0x06u,
    S_LAZY_SYMBOL_POINTERS = 0x07u,
    S_SYMBOL_STUBS = 0x08u,
    S_MOD_INIT_FUNC_POINTERS = 0x09u,
    S_MOD_TERM_FUNC_POINTERS = 0x0au,
    S_COALESCED = 0x0bu,
    S_GB_ZEROFILL = 0x0cu,
    S_INTERPOSING = 0x0du,
    S_16BYTE_LITERALS = 0x0eu,
    S_DTRACE_DOF = 0x0fu,
    S_LAZY_DYLIB_SYMBOL_POINTERS = 0x10u,
    S_THREAD_LOCAL_REGULAR = 0x11u,
    S_THREAD_LOCAL_ZEROFILL = 0x12u,
    S_THREAD_LOCAL_VARIABLES = 0x13u,
    S_THREAD_LOCAL_VARIABLE_POINTERS = 0x14u,
    S_THREAD_LOCAL_INIT_FUNCTION_POINTERS = 0x15u,
    S_INIT_FUNC_OFFSETS = 0x16u,
    LAST_KNOWN_SECTION_TYPE = S_INIT_FUNC_OFFSETS
};

enum : uint32_t {
    SG_HIGHVM = 0x1u,
    SG_FVMLIB = 0x2u,
    SG_NORELOC = 0x4u,
    SG_PROTECTED_VERSION_1 = 0x8u,
    SG_READ_ONLY = 0x10u,
    SECTION_TYPE = 0x000000ffu,
    SECTION_ATTRIBUTES = 0xffffff00u,
    SECTION_ATTRIBUTES_USR = 0xff000000u,
    SECTION_ATTRIBUTES_SYS = 0x00ffff00u
};

enum {
    REFERENCE_TYPE = 0x7,
    REFERENCE_FLAG_UNDEFINED_NON_LAZY = 0,
    REFERENCE_FLAG_UNDEFINED_LAZY = 1,
    REFERENCE_FLAG_DEFINED = 2,
    REFERENCE_FLAG_PRIVATE_DEFINED = 3,
    REFERENCE_FLAG_PRIVATE_UNDEFINED_NON_LAZY = 4,
    REFERENCE_FLAG_PRIVATE_UNDEFINED_LAZY = 5,
    N_ARM_THUMB_DEF = 0x0008u,
    REFERENCED_DYNAMICALLY = 0x0010u,
    N_NO_DEAD_STRIP = 0x0020u,
    N_WEAK_REF = 0x0040u,
    N_WEAK_DEF = 0x0080u,
    N_SYMBOL_RESOLVER = 0x0100u,
    N_ALT_ENTRY = 0x0200u,
    N_COLD_FUNC = 0x0400u,
    SELF_LIBRARY_ORDINAL = 0x0,
    MAX_LIBRARY_ORDINAL = 0xfd,
    DYNAMIC_LOOKUP_ORDINAL = 0xfe,
    EXECUTABLE_ORDINAL = 0xff
};
#endif
