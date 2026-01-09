/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cstring>
#include <cinttypes>
#include <cstddef>
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "app.h"
#include "dir.h"
#include "coff.h"
#include "elf32.h"
#include "elf64.h"
#include "platform.h"
#include "toarray.h"

#ifdef __APPLE__
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/fat.h>
#else
#include "maco.h"
#endif

namespace {
struct FsEntry {
    char fname[256];
    uint64_t offset;
    uint64_t size;
};

std::vector<FsEntry> g_directory;
std::vector<uint8_t> g_bin;
std::vector<std::string_view> g_validExts;

bool AddFile(const std::string& filename, const std::string& storename)
{
    std::string_view ext;
    auto pos = filename.find_last_of('.');
    if (pos != std::string::npos) {
        // found it.
        ext = std::string_view(filename).substr(pos);
    }
    bool valid = false;
    for (const auto& e : g_validExts) {
        if (ext.compare(e) == 0) {
            valid = true;
            break;
        }
    }
    if (!valid) {
        printf("Skipped %s\n", storename.c_str());
        return true;
    }
    if (storename.size() > (sizeof(FsEntry::fname) - 1u)) {
        printf("Filename too long [%s]\n", storename.c_str());
        return false;
    }
    FsEntry tmp{};
    tmp.fname[storename.copy(tmp.fname, sizeof(tmp.fname) - 1U)] = '\0';
#if defined(_WIN32) && (_WIN32)
    struct _stat64 fileStat;
    if (_stat64(filename.c_str(), &fileStat) == -1) {
#else
    struct stat fileStat;
    if (stat(filename.c_str(), &fileStat) == -1) {
#endif
        printf("File [%s] not found\n", tmp.fname);
        return false;
    }
    tmp.size = fileStat.st_size;
    auto padding = (8 - (g_bin.size() & 7)) & 7;
    tmp.offset = g_bin.size() + padding;
    g_directory.push_back(tmp);
    FILE* f = fopen(filename.c_str(), "rb");
    if (f == nullptr) {
        printf("Could not open %s.\n", filename.c_str());
        return false;
    }
    g_bin.resize(size_t(g_bin.size() + padding + tmp.size));
    fread(g_bin.data() + tmp.offset, 1, size_t(tmp.size), f);
    fclose(f);
    printf("Stored: %s [%" PRIu64 " , %" PRIu64 "]\n", tmp.fname, tmp.offset, tmp.size);
    return true;
}

bool AddDirectory(const std::string& path, const std::string& outpath)
{
    struct dirent* pDirent = nullptr;
    DIR* pDir = opendir(path.c_str());
    if (pDir == nullptr) {
        printf("Cannot open directory '%s'\n", path.c_str());
        return false;
    }
    std::string p = path;
    if (!p.empty()) {
        if (p.back() != '/') {
            p += '/';
        }
    }
    std::string op = outpath;
    if (!op.empty()) {
        if (op.back() != '/') {
            op += '/';
        }
    }
    std::vector<dirent> entries;
    while ((pDirent = readdir(pDir)) != nullptr) {
        if (pDirent->d_type == DT_DIR) {
            if (pDirent->d_name[0] == '.') {
                continue;
            }
        }
        entries.push_back(*pDirent);
    }
    std::sort(entries.begin(), entries.end(), [](const dirent& lhs, const dirent& rhs) {
        if (lhs.d_type < rhs.d_type) {
            return true;
        }
        if (lhs.d_type > rhs.d_type) {
            return false;
        }
        return strcmp(lhs.d_name, rhs.d_name) < 0;
    });
    for (const auto& dirEntry : entries) {
        if (dirEntry.d_type == DT_DIR) {
            AddDirectory(p + dirEntry.d_name, op + dirEntry.d_name);
        } else if (!AddFile(p + dirEntry.d_name, op + dirEntry.d_name)) {
            return false;
        }
    }
    closedir(pDir);
    return true;
}

/*
 *   //Pseudo code for accessing files in blob
 *   struct FsEntry
 *   {
 *       char fname[256];
 *       uint64_t offset;
 *       uint64_t size;
 *   };
 *   extern "C" uint64_t SizeOfDataForReadOnlyFileSystem;
 *   extern "C" struct FsEntry BinaryDataForReadOnlyFileSystem[];
 *   void dump_files()
 *   {
 *       for (int i = 0; i < SizeOfDataForReadOnlyFileSystem; i++)
 *       {
 *           if (BinaryDataForReadOnlyFileSystem[i].fname[0] == 0) break;
 *           printf("%s\n", BinaryDataForReadOnlyFileSystem[i].fname);
 *           char* data = (char*)(BinaryDataForReadOnlyFileSystem[i].offset +
 * (uintptr_t)BinaryDataForReadOnlyFileSystem); printf("%lld\n", BinaryDataForReadOnlyFileSystem[i].offset);
 *       }
 *   }
 */

std::string g_dataName = "BinaryDataForReadOnlyFileSystem";
std::string g_sizeName = "SizeOfDataForReadOnlyFileSystem";

#pragma pack(push, 1)
// Using headers and defines from winnt.h
struct ObjFile {
    IMAGE_FILE_HEADER coffHead;
    IMAGE_SECTION_HEADER sections[1];
    IMAGE_SYMBOL symtab[2];
};
#pragma pack(pop)

struct StringTable {
    char table[256u]{};
    char* dst = table;
    StringTable() : dst(table) {}
    uint32_t addString(std::string_view a)
    {
        const auto offset = dst - table;
        dst += a.copy(dst, sizeof(table) - offset);
        *dst++ = '\0';
        return static_cast<uint32_t>(offset);
    }
    ptrdiff_t GetOffset() const
    {
        return dst - table;
    }
};

template<typename T>
void FillCoffHead(T& obj, bool x64)
{
    // fill coff header.
    obj.coffHead.machine = (!x64) ? IMAGE_FILE_MACHINE_I386 : IMAGE_FILE_MACHINE_AMD64;
    obj.coffHead.numberOfSections = sizeof(obj.sections) / sizeof(IMAGE_SECTION_HEADER);
    obj.coffHead.timeDateStamp = 0; // duh.
    obj.coffHead.pointerToSymbolTable = offsetof(T, symtab);
    obj.coffHead.numberOfSymbols = sizeof(obj.symtab) / sizeof(IMAGE_SYMBOL);
    obj.coffHead.sizeOfOptionalHeader = 0;
    obj.coffHead.characteristics = 0; // if x86 use IMAGE_FILE_32BIT_MACHINE ?
}

template<typename T>
size_t FillCoffSymbtable(T& obj, StringTable& stringtable, bool x64)
{
    if (!x64) {
        // in win32 the symbols have extra "_" ?
        std::string t = "_";
        t += g_dataName;
        std::string t2 = "_";
        t2 += g_sizeName;
        obj.symtab[1].n.name.Long = stringtable.addString(t);
        obj.symtab[0].n.name.Long = stringtable.addString(t2);
    } else {
        obj.symtab[1].n.name.Long = stringtable.addString(g_dataName);
        obj.symtab[0].n.name.Long = stringtable.addString(g_sizeName);
    }

    const uint32_t stringTableSize = uint32_t(stringtable.GetOffset());
    *reinterpret_cast<uint32_t*>(stringtable.table) = stringTableSize;
    return stringTableSize;
}

template<typename T>
void FillCoffSectionAndSymtable(
    T& obj, size_t stringTableSize, const size_t sizeOfSection, const std::string& secname, bool x64)
{
    // fill the section.
    secname.copy(reinterpret_cast<char*>(obj.sections[0].name), sizeof(obj.sections[0].name));
    obj.sections[0].misc.virtualSize = 0;
    obj.sections[0].virtualAddress = 0;
    obj.sections[0].sizeOfRawData = uint32_t(sizeOfSection); // sizeof the data on disk.
    obj.sections[0].pointerToRawData = static_cast<uint32_t>(
        ((sizeof(T) + stringTableSize + 3) / 4) * 4); // DWORD align the data directly after the headers..
    obj.sections[0].pointerToLinenumbers = 0;
    obj.sections[0].numberOfRelocations = 0;
    obj.sections[0].numberOfLinenumbers = 0;
    obj.sections[0].characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ;
    // fill symbols
    obj.symtab[1].value = uint32_t(sizeof(uint64_t));
    obj.symtab[1].sectionNumber = 1; // first section.. (one based)
    obj.symtab[1].type = IMAGE_SYM_TYPE_CHAR | (IMAGE_SYM_DTYPE_ARRAY << 8);
    obj.symtab[1].storageClass = IMAGE_SYM_CLASS_EXTERNAL;
    obj.symtab[1].numberOfAuxSymbols = 0;

    obj.symtab[0].value = uint32_t(0u);
    obj.symtab[0].sectionNumber = 1; // first section.. (one based)
    // obj.symtab[0].type = IMAGE_SYM_TYPE_UINT;  //(just use IMAGE_SYM_TYPE_NULL like mstools?)
    obj.symtab[0].storageClass = IMAGE_SYM_CLASS_EXTERNAL;
    obj.symtab[0].numberOfAuxSymbols = 0;
}

bool WriteObj(const std::string& fname, const std::string& secname, size_t sizeOfData, const void* data, bool x64)
{
    const size_t sizeOfSection = sizeof(uint64_t) + sizeOfData;
    ObjFile obj{};
    StringTable stringtable;
    stringtable.dst += sizeof(uint32_t); // leave space for the size of the table as uint32

    FillCoffHead(obj, x64);
    size_t stringTableSize = FillCoffSymbtable(obj, stringtable, x64);
    FillCoffSectionAndSymtable(obj, static_cast<size_t>(stringTableSize), sizeOfSection, secname, x64);

    FILE* d = fopen(fname.c_str(), "wb");
    if (d == nullptr) {
        printf("Could not open %s.\n", fname.c_str());
        return false;
    }
    // write headers
    fwrite(&obj, sizeof(obj), 1u, d);
    // write string table
    fwrite(stringtable.table, stringTableSize, 1u, d);
    // write sections..
    size_t p = ftell(d);
    uint32_t pad = 0;
    size_t padcount = obj.sections[0].pointerToRawData - p;
    fwrite(&pad, padcount, 1u, d);
    fwrite(data, sizeOfSection, 1u, d);
    fclose(d);

    return true;
}

struct Elf32Bit {
    Elf32_Ehdr head;
    Elf32_Shdr sections[4];
    Elf32_Sym symbs[3];
};

struct Elf64Bit {
    Elf64_Ehdr head;
    Elf64_Shdr sections[4];
    Elf64_Sym symbs[3];
};

template<class Type>
void FillElfHead(Type& o, uint8_t arch)
{
    o.head.type = ET_REL;
    o.head.machine = arch; // machine id..
    o.head.version = EV_CURRENT;
    o.head.ehsize = sizeof(o.head);
    o.head.shentsize = sizeof(o.sections[0]);
    o.head.shnum = sizeof(o.sections) / sizeof(o.sections[0]);
    o.head.shoff = sizeof(o.head);
    o.head.shstrndx = 1;
}

template<class Type>
void FillElfSectionAndSymbtable(Type& o, uint8_t arch, StringTable& stringtable, size_t sizeOfData)
{
    // create symbols
    o.symbs[2].name = stringtable.addString(g_dataName); // ?BinaryDataForReadOnlyFileSystem@@3PADA");
    o.symbs[2].value = sizeof(uint64_t);
    o.symbs[2].size = static_cast<Elf32_Word>(sizeOfData);
    o.symbs[2].info = o.symbs[1].info = ELF_ST_INFO(STB_GLOBAL, STT_OBJECT);
    o.symbs[2].other = o.symbs[1].other = STV_HIDDEN;
    o.symbs[2].shndx = o.symbs[1].shndx = 3;

    o.symbs[1].name = stringtable.addString(g_sizeName);
    o.symbs[1].value = 0;
    o.symbs[1].size = sizeof(uint64_t);

    o.sections[2].name = stringtable.addString(".symtab");
    o.sections[2].type = SHT_SYMTAB;
    o.sections[2].offset = offsetof(Type, symbs); // sizeof(o) + sizeOfSection + stringtable_size;
    o.sections[2].addralign = 8;
    o.sections[2].size = sizeof(o.symbs);
    o.sections[2].entsize = sizeof(o.symbs[0]);
    o.sections[2].link = 1;
    o.sections[2].info = 1; // index of first non-local symbol.
}

template<class Type>
void FillElfSection(Type& o, uint8_t arch, const std::string& secname, StringTable& stringtable, size_t sizeOfSection)
{
    std::string tmp = ".rodata.";
    tmp += secname;

    o.sections[3].name = stringtable.addString(tmp.data());
    o.sections[3].type = SHT_PROGBITS;
    o.sections[3].flags = SHF_ALLOC | SHF_MERGE;
    o.sections[3].offset = sizeof(o);
    o.sections[3].addralign = 8;
    o.sections[3].size = static_cast<Elf32_Word>(sizeOfSection);

    o.sections[1].name = stringtable.addString(".strtab");
    o.sections[1].type = SHT_STRTAB;
    o.sections[1].offset = static_cast<Elf32_Off>(sizeof(o) + sizeOfSection);
    o.sections[1].addralign = 1;
    o.sections[1].size = static_cast<Elf32_Word>(stringtable.GetOffset());
}

template<class Type>
bool WriteElf(uint8_t arch, const std::string& fname, const std::string& secname, size_t sizeOfData, const void* data)
{
    const size_t sizeOfSection = sizeOfData + sizeof(uint64_t);
    Type o{};
    FillElfHead(o, arch);

    // initialize stringtable.
    StringTable stringtable;
    *(stringtable.dst) = '\0';
    ++(stringtable.dst);

    FillElfSectionAndSymbtable(o, arch, stringtable, sizeOfData);

    FillElfSection(o, arch, secname, stringtable, sizeOfSection);

    FILE* e = fopen(fname.c_str(), "wb");
    if (e == nullptr) {
        printf("Could not open %s.\n", fname.c_str());
        return false;
    }
    fwrite(&o, sizeof(o), 1, e);
    fwrite(data, sizeOfSection, 1, e);
    fwrite(stringtable.table, size_t(o.sections[1].size), 1, e);
    fclose(e);
    return true;
}

// Note: Assumes power of two alignment.
size_t GetAligned(size_t offset, size_t alignment)
{
    return (offset + (alignment - 1)) & ~(alignment - 1);
}

auto MachoHeader()
{
    FatHeader fathdr;
    fathdr.magic = FAT_CIGAM;
    fathdr.nfat_arch = platform_htonl(2); // big-endian values in fat header
    return fathdr;
}

auto MachoArchs(uint32_t fatAlignmentPowerOfTwo)
{
    using namespace std;
    FatArch archs[]{
        {
            static_cast<CpuType>(platform_htonl(CPU_TYPE_X86_64)),
            static_cast<CpuSubtype>(platform_htonl(CPU_SUBTYPE_X86_64_ALL)),
            0, // offset
            0, // size of data
            platform_htonl(fatAlignmentPowerOfTwo),
        },
        {
            static_cast<CpuType>(platform_htonl(CPU_TYPE_ARM64)),
            static_cast<CpuSubtype>(platform_htonl(CPU_SUBTYPE_ARM64_ALL)),
            0, // offset,
            0, // size of data
            platform_htonl(fatAlignmentPowerOfTwo),
        },
    };

    return to_array(archs);
}

auto MachoX64Header()
{
    MachHeader64 x64_header = {
        MH_MAGIC_64, static_cast<CpuType>(CPU_TYPE_X86_64), static_cast<CpuSubtype>(CPU_SUBTYPE_X86_64_ALL), MH_OBJECT,
        2,                                                                    // ncmds
        sizeof(SegmentCommand64) + sizeof(Section64) + sizeof(SymtabCommand), // sizeofcmds
        0,                                                                    // flags
        0                                                                     // reserved
    };

    return x64_header;
}

auto MachoARM64Header()
{
    MachHeader64 arm64_header = {
        MH_MAGIC_64, static_cast<CpuType>(CPU_TYPE_ARM64), static_cast<CpuSubtype>(CPU_SUBTYPE_ARM64_ALL), MH_OBJECT,
        2,                                                                    // ncmds
        sizeof(SegmentCommand64) + sizeof(Section64) + sizeof(SymtabCommand), // sizeofcmds
        0,                                                                    // flags
        0                                                                     // reserved
    };

    return arm64_header;
}

auto MachoSegmentCommand64(size_t& sizeOfSection)
{
    SegmentCommand64 data_seg = {
        LC_SEGMENT_64, sizeof(data_seg) + sizeof(Section64),
        "",                       // for object files name is empty
        0,                        // vmaddress
        (sizeOfSection + 7) & ~7, // vmsize aligned to 8 bytes
        0,                        // fileoffset
        sizeOfSection,            // filesize
        VM_PROT_READ,             // maxprot
        VM_PROT_READ,             // initprot
        1,                        // nsects
        SG_NORELOC                // flags
    };

    return data_seg;
}

auto MachoDataSection(size_t& sizeOfSection)
{
    Section64 data_sect = {
        "__const", "__DATA",
        0,             // addr
        sizeOfSection, // vmsize aligned to 8 bytes
        0,             // offset
        3,             // alignment = 2^3 = 8 bytes
        0,             // reloff
        0,             // nreloc
        S_REGULAR,     // flags
        0,             // reserved1
        0,             // reserved2
    };

    return data_sect;
}

auto MachoSymbolTable(uint32_t& string_size)
{
    SymtabCommand symtab = {
        LC_SYMTAB, sizeof(symtab),
        0,           // symoff
        2,           // nsyms
        0,           // stroff
        string_size, // strsize
    };

    return symtab;
}

auto MachoList(std::string& sizeName)
{
    using namespace std;
    Nlist64 ret[]{
        {
            1, // first string
            N_EXT | N_SECT,
            1, // segment
            REFERENCE_FLAG_DEFINED,
            0,
        },
        {
            static_cast<uint32_t>(sizeName.size() + 2), // second string
            N_EXT | N_SECT,
            1, // segment
            REFERENCE_FLAG_DEFINED,
            8,
        },
    };

    return to_array(ret);
}

struct MachoPaddingInfo {
    uint32_t offset0;
    uint32_t offsetAligned0;
    uint32_t padding0;

    uint32_t offset1;
    uint32_t offsetAligned1;
    uint32_t padding1;
};

auto MachoUpdate(FatHeader& fathdr, FatArch* archs, uint32_t fatAlignment, size_t fpos)
{
    const uint32_t sliceOffset0 = static_cast<uint32_t>(sizeof(fathdr) + sizeof(archs)); // initial headers
    const uint32_t sliceOffsetAligned0 = static_cast<uint32_t>(GetAligned(sliceOffset0, fatAlignment));
    const uint32_t slicePadding0 = sliceOffsetAligned0 - sliceOffset0;

    archs[0].offset = platform_htonl(sliceOffsetAligned0);
    archs[0].size = platform_htonl(static_cast<uint32_t>(fpos));

    const uint32_t sliceOffset1 = static_cast<uint32_t>(sliceOffsetAligned0 + fpos);
    const uint32_t sliceOffsetAligned1 = static_cast<uint32_t>(GetAligned(sliceOffset1, fatAlignment));
    const uint32_t slicePadding1 = sliceOffsetAligned1 - sliceOffset1;

    archs[1].offset = platform_htonl(sliceOffsetAligned1);
    archs[1].size = platform_htonl(static_cast<uint32_t>(fpos));

    return MachoPaddingInfo{ sliceOffset0, sliceOffsetAligned0, slicePadding0, sliceOffset1, sliceOffsetAligned1,
        slicePadding1 };
}

struct Macho {
    FatHeader fathdr;
    std::array<FatArch, 2> archs; // 2: size
    MachHeader64 x64Header;
    MachHeader64 arm64Header;
    SegmentCommand64 dataSeg;
    Section64 dataSect;
    SymtabCommand symtab;
    std::array<Nlist64, 2> syms; // 2: size
};

template<typename T1>
void MachoWriteArchitechtureData(size_t padding, size_t sizeOfSection, size_t sectionAlign, const std::string& dataName,
    const std::string& sizeName, const void* data, const T1& sect, const MachHeader64& header, FILE* e)
{
    for (size_t i = 0; i < padding; i++) {
        fputc(0, e);
    }

    fwrite(&header, sizeof(header), 1, e);
    fwrite(&sect.dataSeg, sizeof(sect.dataSeg), 1, e);
    fwrite(&sect.dataSect, sizeof(sect.dataSect), 1, e);
    fwrite(&sect.symtab, sizeof(sect.symtab), 1, e);
    fwrite(data, sizeOfSection, 1, e);
    for (size_t i = 0; i < sectionAlign; i++) {
        fputc(0, e); // alignment byte to begin string table 8 byte boundary
    }
    fwrite(&sect.syms, sizeof(sect.syms), 1, e);
    fputc(0, e); // filler byte to begin string table as it starts indexing at 1
    fwrite(sizeName.c_str(), sizeName.size() + 1, 1, e);
    fwrite(dataName.c_str(), dataName.size() + 1, 1, e);
}

template<typename T1>
bool MachoWriteFile(const std::string& fname, const T1& sect, const MachoPaddingInfo& slices,
    const std::string& dataName, const std::string& sizeName, const void* data, size_t endPadding, size_t sectionAlign,
    size_t sizeOfSection)
{
    FILE* e = fopen(fname.c_str(), "wb");
    if (e == nullptr) {
        printf("Could not open %s.\n", fname.c_str());
        return false;
    }

    fwrite(&sect.fathdr, sizeof(sect.fathdr), 1, e);
    fwrite(&sect.archs, sizeof(sect.archs), 1, e);
    MachoWriteArchitechtureData(
        slices.padding0, sizeOfSection, sectionAlign, dataName, sizeName, data, sect, sect.x64Header, e);
    MachoWriteArchitechtureData(
        slices.padding0, sizeOfSection, sectionAlign, dataName, sizeName, data, sect, sect.arm64Header, e);
    for (uint32_t i = 0; i < endPadding; i++) {
        fputc(0, e);
    }

    fclose(e);

    return true;
}

std::string PrefixUnderscore(std::string& input)
{
    std::string tmp = "_";
    tmp += input;
    return tmp;
}

size_t CalculateAlignment(std::size_t& fpos, uint32_t aligment)
{
    size_t sectionAligned = GetAligned(static_cast<uint32_t>(fpos), aligment);
    size_t sectionAlign = sectionAligned - fpos;
    fpos = sectionAligned;
    return sectionAlign;
}

bool WriteMacho(const std::string& fname, const std::string& secname, size_t sizeOfData, const void* data)
{
    // The fat slices need to be page aligned (to 16k pages), 16k alignment = 2^14
    constexpr uint32_t fatAlignmentPowerOfTwo = 14, fatAlignment = 1 << fatAlignmentPowerOfTwo;

    Macho sect;
    sect.fathdr = MachoHeader();
    sect.archs = MachoArchs(fatAlignmentPowerOfTwo);

    // "file" offsets are actually relative to the architecture header
    size_t fpos = 0, sizeOfSection = sizeOfData + sizeof(uint64_t);

    sect.x64Header = MachoX64Header(), sect.arm64Header = MachoARM64Header();
    sect.dataSeg = MachoSegmentCommand64(sizeOfSection);
    sect.dataSect = MachoDataSection(sizeOfSection);

    std::string dataName = PrefixUnderscore(g_dataName), sizeName = PrefixUnderscore(g_sizeName);

    uint32_t string_size =
        static_cast<uint32_t>(dataName.size() + sizeName.size() + 3); // prepending plus two terminating nulls

    sect.symtab = MachoSymbolTable(string_size);

    fpos += sizeof(sect.x64Header) + sizeof(sect.dataSeg) + sizeof(sect.dataSect) + sizeof(sect.symtab);

    sect.dataSeg.fileOff = sect.dataSect.offset = static_cast<uint32_t>(fpos);
    fpos += sizeOfSection;

    size_t sectionAlign = CalculateAlignment(fpos, 8);
    sect.symtab.symoff = static_cast<uint32_t>(fpos);
    auto syms = MachoList(sizeName);
    fpos += sizeof(syms);

    sect.symtab.stroff = static_cast<uint32_t>(fpos);
    fpos += string_size;

    auto slices = MachoUpdate(sect.fathdr, sect.archs.data(), fatAlignment, fpos);
    const size_t endPadding = GetAligned(slices.offsetAligned1 + fpos, fatAlignment);

    return MachoWriteFile(fname, sect, slices, dataName, sizeName, data, endPadding, sectionAlign, sizeOfSection);
}

enum Arguments {
    EXECUTABLE,
    SOURCE_DIR = 1,
    TARGET_DIR = 2,
    ROFS_DATA_NAME = 3,
    ROFS_SIZE_NAME = 4,
    FILE_NAME = 5,
};
enum Arch : uint32_t {
    BUILD_X86 = (1 << 0),
    BUILD_X64 = (1 << 1),
    BUILD_V7 = (1 << 2),
    BUILD_V8 = (1 << 3),
};
enum Plat : uint32_t {
    WINDOWS = (1 << 4),
    ANDROID = (1 << 5),
    MAC = (1 << 6),
};

constexpr uint32_t PlatformSet()
{
    return 1U << 31U;
}

constexpr std::pair<std::string_view, uint32_t> PLATFORMS[] = {
    { "-linux", ANDROID | PlatformSet() },
    { "-android", ANDROID | PlatformSet() },
    { "-windows", WINDOWS | PlatformSet() },
    { "-mac", MAC | PlatformSet() },
    { "-x86", BUILD_X86 },
    { "-x86_64", ANDROID | BUILD_X64 },
    { "-x64", WINDOWS | BUILD_X64 },
    { "-armeabi-v7a", ANDROID | BUILD_V7 | PlatformSet() },
    { "-arm64-v8a", ANDROID | BUILD_V8 | PlatformSet() },
};

void ParseExtensions(std::string_view exts, std::vector<std::string_view>& extensions)
{
    while (!exts.empty()) {
        auto pos = exts.find(';');
        pos = std::min(pos, exts.size());
        extensions.push_back(exts.substr(0, pos));
        exts.remove_prefix(std::min(pos + 1, exts.size()));
    }
}

int ParseArcAndPlat(uint32_t& arcAndPlat, const int argc, char* argv[])
{
    if (argc <= 1) {
        return -1;
    }

    arcAndPlat = argv[1][0] == '-' ? 0u : arcAndPlat;

    int baseArg = 0;
    bool platset = false;
    for (;;) {
        if (baseArg + 1 >= argc) {
            printf("Invalid argument!\n");
            return -1;
        }
        if (argv[baseArg + 1][0] != '-') {
            break;
        }

        if (auto pos = std::find_if(std::begin(PLATFORMS), std::end(PLATFORMS),
                [argument = argv[baseArg + 1]](
                    const std::pair<std::string_view, uint32_t>& item) { return item.first == argument; });
            pos != std::end(PLATFORMS)) {
            arcAndPlat |= pos->second & (~PlatformSet());
            platset |= (pos->second & PlatformSet()) != 0;
            baseArg++;
        } else if (strcmp(argv[baseArg + 1], "-extensions") == 0) {
            baseArg++;
            if (baseArg + 1 >= argc) {
                printf("Invalid argument!\n");
                return -1;
            }
            auto exts = std::string_view(argv[baseArg + 1]);
            ParseExtensions(exts, g_validExts);
            baseArg++;
        } else {
            printf("Invalid argument!\n");
            return -1;
        }
    }

    arcAndPlat |= (platset ? 0 : (WINDOWS | ANDROID | MAC));
    return baseArg;
}
} // namespace

int app_main(int argc, char* argv[])
{
    if (argc <= 1) {
        printf("Not enough arguments!\n");
        return -1;
    }

    uint32_t arcAndPlat = (BUILD_X86 | BUILD_X64 | BUILD_V7 | BUILD_V8) | (WINDOWS | ANDROID | MAC);
    int baseArg = ParseArcAndPlat(arcAndPlat, argc, argv);
    if (baseArg < 0) {
        return -1;
    }
    if (argc < (baseArg + ROFS_DATA_NAME)) {
        printf("invalid args");
        return -1;
    }
    std::string inPath = argv[baseArg + SOURCE_DIR];

    std::string roPath;
    if (argv[baseArg + TARGET_DIR][0] == '/') {
        roPath = argv[baseArg + TARGET_DIR] + 1;
    } else {
        roPath = argv[baseArg + TARGET_DIR];
    }

    if (argc > (baseArg + ROFS_SIZE_NAME)) {
        g_dataName = argv[baseArg + ROFS_DATA_NAME];
        g_sizeName = argv[baseArg + ROFS_SIZE_NAME];
    }

    std::string obj32Name = "rofs_32.obj";
    std::string obj64Name = "rofs_64.obj";
    std::string o32Name = "rofs_32.o";
    std::string o64Name = "rofs_64.o";
    std::string x32Name = "rofs_x86.o";
    std::string x64Name = "rofs_x64.o";
    std::string macName = "rofs_mac.o";
    std::string secName = "rofs";

    if (argc > (baseArg + FILE_NAME)) {
        secName = obj32Name = obj64Name = o32Name = o64Name = x32Name = x64Name = macName = argv[baseArg + FILE_NAME];
        obj32Name += "_32.obj";
        obj64Name += "_64.obj";
        o32Name += "_32.o";
        o64Name += "_64.o";
        x32Name += "_x86.o";
        x64Name += "_x64.o";
        macName += "_mac.o";
    }
    if (!AddDirectory(inPath, roPath)) {
        return -1;
    }

    // fix offsets
    size_t baseOffset = sizeof(FsEntry) * (g_directory.size() + 1);
    for (auto& d : g_directory) {
        d.offset += baseOffset;
    }

    // add terminator
    g_directory.push_back({ { 0 }, 0, 0 });

    const size_t sizeOfDir = (sizeof(FsEntry) * g_directory.size());
    const size_t sizeOfData = g_bin.size() + sizeOfDir;
    auto data = std::make_unique<uint8_t[]>(sizeOfData + sizeof(uint64_t));
    *reinterpret_cast<uint64_t*>(data.get()) = sizeOfData;
    std::copy(g_directory.cbegin(), g_directory.cend(), reinterpret_cast<FsEntry*>(data.get() + sizeof(uint64_t)));
    std::copy(g_bin.cbegin(), g_bin.cend(), data.get() + sizeof(uint64_t) + sizeOfDir);

    // Build obj
    if (arcAndPlat & WINDOWS) {
        if (arcAndPlat & BUILD_X86) {
            if (!WriteObj(obj32Name, secName, sizeOfData, data.get(), false)) {
                return -1;
            }
        }
        if (arcAndPlat & BUILD_X64) {
            if (!WriteObj(obj64Name, secName, sizeOfData, data.get(), true)) {
                return -1;
            }
        }
    }
    // Create .elf (.o)
    if (arcAndPlat & ANDROID) {
        if (arcAndPlat & BUILD_V7) {
            if (!WriteElf<Elf32Bit>(EM_ARM, o32Name, secName, sizeOfData, data.get())) {
                return -1;
            }
        }
        if (arcAndPlat & BUILD_V8) {
            if (!WriteElf<Elf64Bit>(EM_AARCH64, o64Name, secName, sizeOfData, data.get())) {
                return -1;
            }
        }
        if (arcAndPlat & BUILD_X86) {
            if (!WriteElf<Elf32Bit>(EM_386, x32Name, secName, sizeOfData, data.get())) {
                return -1;
            }
        }
        if (arcAndPlat & BUILD_X64) {
            if (!WriteElf<Elf64Bit>(EM_X86_64, x64Name, secName, sizeOfData, data.get())) {
                return -1;
            }
        }
    }
    // Create mach-o (.o)
    if (arcAndPlat & MAC) {
        if (!WriteMacho(macName, secName, sizeOfData, data.get())) {
            return -1;
        }
    }
    return 0;
}
