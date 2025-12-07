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

#ifndef LUME_MACO_H
#define LUME_MACO_H

// Using  https://llvm.org/doxygen/structllvm_1_1MachO_1_1fat__arch.html for publicly documented structure definition

enum : uint32_t {
    LC_SEGMENT_64 = 0x19,
    LC_SYMTAB = 0x2,
};

// Constants for the "n_type & N_TYPE" llvm::MachO::nlist and llvm::MachO::nlist_64
enum : uint8_t {
    N_SECT = 0xeu,
};

// Constants for the "filetype" field in llvm::MachO::mach_header and  llvm::MachO::mach_header_64
enum : uint32_t {
    MH_OBJECT = 0x1u,
};

enum : uint32_t {
    CPU_ARCH_ABI64 = 0x01000000, // 64 bit ABI
};

// Constants for the cputype field.
enum : uint32_t {
    CPU_TYPE_X86 = 7,
    CPU_TYPE_X86_64 = CPU_TYPE_X86 | CPU_ARCH_ABI64,
    CPU_TYPE_ARM = 12,
    CPU_TYPE_ARM64 = CPU_TYPE_ARM | CPU_ARCH_ABI64,
};

enum : uint32_t {
    CPU_SUBTYPE_X86_64_ALL = 3,
};

enum : uint32_t {
    CPU_SUBTYPE_ARM64_ALL = 0,
};

enum : uint32_t {
    MH_MAGIC_64 = 0xFEEDFACFu,
    FAT_CIGAM = 0xBEBAFECAu,
};

enum { N_EXT = 0x01 };

enum { VM_PROT_READ = 0x1 };

enum CpuSubtype {};
enum CpuType {};
typedef int vm_prot_t;

struct FatHeader {
    uint32_t magic;
    uint32_t nfat_arch;
};

struct FatArch {
    CpuType cputype;
    CpuSubtype cpusubtype;
    uint32_t offset;
    uint32_t size;
    uint32_t align;
};

struct MachHeader64 {
    uint32_t magic;

    CpuType cputype;

    CpuSubtype cpusubtype;

    uint32_t filetype;

    uint32_t ncmds;

    uint32_t sizeofcmds;

    uint32_t flags;

    uint32_t reserved;
};

struct SymtabCommand {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;
    uint32_t symoff;
};

struct Section64 {
    char sectName[16];
    char segName[16];
    uint64_t addr;
    uint64_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t relOff;
    uint32_t nReloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
};

struct SegmentCommand64 {
    uint32_t cmd;
    uint32_t cmdSize;
    char segName[16];
    uint64_t vmAddr;
    uint64_t vmSize;
    uint64_t fileOff;
    uint64_t fileSize;
    uint32_t maxProt;
    uint32_t initProt;
    uint32_t nSects;
    uint32_t flags;
};

struct Nlist64 {
    uint32_t strx;
    uint8_t type;
    uint8_t sect;
    uint16_t desc;
    uint64_t value;
};

// These are the section type and attributes fields.  A MachO section can have only one Type, but can have any of the
// attributes specified.
// Constant masks for the "flags[7:0]" field in llvm::MachO::section and llvm::MachO::section_64 (mask "flags" with
// SECTION_TYPE)
enum SectionType : uint32_t {
    S_REGULAR = 0x00u,
};

// Constant bits for the "flags" field in llvm::MachO::segment_command
enum : uint32_t {
    SG_NORELOC = 0x4u,
};

enum {
    REFERENCE_FLAG_DEFINED = 2,
};
#endif // LUME_MACO_H
