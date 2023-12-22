/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include <cstddef>
#include <string.h>
#include <inttypes.h>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include "dir.h"
#include "coff.h"
#include "elf32.h"
#include "elf64.h"

#ifdef __APPLE__
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/fat.h>
#endif

struct fs_entry {
    char fname[256];
    uint64_t offset;
    uint64_t size;
};

std::vector<fs_entry> directory;
std::vector<uint8_t> bin;
std::vector<std::string_view> valid_exts;

void append_file(const std::string& filename, const std::string& storename)
{
    std::string_view ext;
    auto pos = filename.find_last_of(".");
    if (pos != std::string::npos) {
        // found it.
        ext = std::string_view(filename).substr(pos);
    }
    bool valid = false;
    for (const auto& e : valid_exts) {
        if (ext.compare(e) == 0) {
            valid = true;
            break;
        }
    }
    if (!valid) {
        printf("Skipped %s\n", storename.c_str());
        return;
    }
    if (storename.size() > 255) {
        printf("Filename too long [%s]\n", storename.c_str());
        exit(-1);
    }
    fs_entry tmp{};
    strcpy(tmp.fname, storename.c_str());
#if _WIN32
    struct _stat64 fileStat;
    if (-1 == _stat64(filename.c_str(), &fileStat)) {
#else
    struct stat fileStat;
    if (-1 == stat(filename.c_str(), &fileStat)) {
#endif
        printf("File [%s] not found\n", tmp.fname);
        exit(-1);
    }
    tmp.size = fileStat.st_size;
    auto padding = (8 - (bin.size() & 7)) & 7;
    tmp.offset = bin.size() + padding;
    directory.push_back(tmp);
    FILE* f = fopen(filename.c_str(), "rb");
    if (f == NULL) {
        printf("Could not open %s.\n", filename.c_str());
        exit(-1);
    }
    bin.resize((size_t)(bin.size() + padding + tmp.size));
    fread(bin.data() + tmp.offset, 1, (size_t)tmp.size, f);
    fclose(f);
    printf("Stored: %s [%" PRIu64 " , %" PRIu64 "]\n", tmp.fname, tmp.offset, tmp.size);
}

void add_directory(const std::string& path, const std::string& outpath)
{
    struct dirent* pDirent = nullptr;
    DIR* pDir = opendir(path.c_str());
    if (pDir == NULL) {
        printf("Cannot open directory '%s'\n", path.c_str());
        exit(1);
    }
    std::string p, op;
    p = path;
    if (!p.empty()) {
        if (p.back() != '/') {
            p += "/";
        }
    }
    op = outpath;
    if (!op.empty()) {
        if (op.back() != '/') {
            op += "/";
        }
    }

    // sort the readdir result
    auto alphaSort = [](dirent* x, dirent* y) { return std::string(x->d_name) < std::string(y->d_name); };
    auto dirSet = std::set<dirent*, decltype(alphaSort)>(alphaSort);

    while ((pDirent = readdir(pDir)) != NULL) {
        // This structure may be statically allocated
        dirSet.insert(pDirent);
    }

    for (auto &d : dirSet) {
        if (d->d_type == DT_DIR) {
            if (d->d_name[0] == '.') {
                continue;
            }
            add_directory(p + d->d_name, op + d->d_name);
            continue;
        }
        append_file(p + d->d_name, op + d->d_name);
    }

    closedir(pDir);
}
/*
 *   //Pseudo code for accessing files in blob
 *   struct fs_entry
 *   {
 *       char fname[256];
 *       uint64_t offset;
 *       uint64_t size;
 *   };
 *   extern "C" uint64_t SizeOfDataForReadOnlyFileSystem;
 *   extern "C" struct fs_entry BinaryDataForReadOnlyFileSystem[];
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

std::string gDataName = "BinaryDataForReadOnlyFileSystem";
std::string gSizeName = "SizeOfDataForReadOnlyFileSystem";

void write_obj(const std::string& fname, const std::string& secname, size_t size_of_data, const void* data, bool x64)
{
    size_t size_of_section = sizeof(uint64_t) + size_of_data;
#pragma pack(push, 1)
    // Using headers and defines from winnt.h
    struct ObjFile {
        IMAGE_FILE_HEADER coffHead;
        IMAGE_SECTION_HEADER sections[1];
        IMAGE_SYMBOL symtab[2];
    } obj{};
#pragma pack(pop)

    // fill coff header.
    if (!x64)
        obj.coffHead.Machine = IMAGE_FILE_MACHINE_I386;
    else
        obj.coffHead.Machine = IMAGE_FILE_MACHINE_AMD64;
    obj.coffHead.NumberOfSections = sizeof(obj.sections) / sizeof(IMAGE_SECTION_HEADER);
    obj.coffHead.TimeDateStamp = 0; // duh.
    obj.coffHead.PointerToSymbolTable = offsetof(decltype(obj), symtab);
    obj.coffHead.NumberOfSymbols = sizeof(obj.symtab) / sizeof(IMAGE_SYMBOL);
    obj.coffHead.SizeOfOptionalHeader = 0;
    obj.coffHead.Characteristics = 0; // if x86 use IMAGE_FILE_32BIT_MACHINE ?

    // create stringtable
    char stringtable[256]{ 0 };
    char* dst = stringtable;
    auto add_string = [&dst, &stringtable](std::string_view a) -> Elf32_Word {
        const auto offset = dst - stringtable;
        dst += a.copy(dst, 256u - offset);
        *dst++ = '\0';
        return static_cast<uint32_t>(offset + 4);
    };

    // obj.symtab[0].N.Name.Long = add_string("?BinaryDataForReadOnlyFileSystem@@3PAUfs_entry@@A");//
    // ?BinaryDataForReadOnlyFileSystem@@3PADA"); obj.symtab[1].N.Name.Long =
    // add_string("?SizeOfDataForReadOnlyFileSystem@@3KA");
    if (!x64) {
        // in win32 the symbols have extra "_" ?
        std::string t = "_";
        t += gDataName;
        std::string t2 = "_";
        t2 += gSizeName;
        obj.symtab[1].N.Name.Long = add_string(t.c_str() /*"BinaryDataForReadOnlyFileSystem"*/);
        obj.symtab[0].N.Name.Long = add_string(t2.c_str() /*"SizeOfDataForReadOnlyFileSystem"*/);
    } else {
        obj.symtab[1].N.Name.Long = add_string(gDataName.c_str() /*"BinaryDataForReadOnlyFileSystem"*/);
        obj.symtab[0].N.Name.Long = add_string(gSizeName.c_str() /*"SizeOfDataForReadOnlyFileSystem"*/);
    }
    uint32_t stringTableSize = (uint32_t)((dst - stringtable) + 4);

    // fill the section.
    memcpy(&obj.sections[0].Name[0], secname.c_str(), secname.size());
    obj.sections[0].Misc.VirtualSize = 0;
    obj.sections[0].VirtualAddress = 0;
    obj.sections[0].SizeOfRawData = (uint32_t)size_of_section; // sizeof the data on disk.
    obj.sections[0].PointerToRawData =
        ((sizeof(obj) + stringTableSize + 3) / 4) * 4; // DWORD align the data directly after the headers..
    obj.sections[0].PointerToLinenumbers = 0;
    obj.sections[0].NumberOfRelocations = 0;
    obj.sections[0].NumberOfLinenumbers = 0;
    obj.sections[0].Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_4BYTES | IMAGE_SCN_MEM_READ;
    // fill symbols
    obj.symtab[1].Value = (uint32_t)sizeof(uint64_t);
    obj.symtab[1].SectionNumber = 1; // first section.. (one based)
    obj.symtab[1].Type = IMAGE_SYM_TYPE_CHAR | (IMAGE_SYM_DTYPE_ARRAY << 8);
    obj.symtab[1].StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    obj.symtab[1].NumberOfAuxSymbols = 0;

    obj.symtab[0].Value = (uint32_t)0;
    obj.symtab[0].SectionNumber = 1; // first section.. (one based)
    // obj.symtab[0].Type = IMAGE_SYM_TYPE_UINT;  //(just use IMAGE_SYM_TYPE_NULL like mstools?)
    obj.symtab[0].StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    obj.symtab[0].NumberOfAuxSymbols = 0;

    FILE* d = fopen(fname.c_str(), "wb");
    if (d == NULL) {
        printf("Could not open %s.\n", fname.c_str());
        exit(-1);
    }
    // write headers
    fwrite(&obj, sizeof(obj), 1, d);
    // write string table
    fwrite(&stringTableSize, 1, sizeof(stringTableSize), d);
    fwrite(stringtable, 1, stringTableSize - 4, d);
    // write sections..
    size_t p = ftell(d);
    uint32_t pad = 0;
    size_t padcount = obj.sections[0].PointerToRawData - p;
    fwrite(&pad, padcount, 1, d);
    fwrite(data, size_of_section, 1, d);
    fclose(d);
#undef add_string
}

template<class type>
void write_elf(
    type o, uint8_t arch, const std::string& fname, const std::string& secname, size_t size_of_data, const void* data)
{
    size_t size_of_section = size_of_data + sizeof(uint64_t);
    char stringtable[256];
    o.head.e_type = ET_REL;
    o.head.e_machine = arch; // machine id..
    o.head.e_version = EV_CURRENT;
    o.head.e_ehsize = sizeof(o.head);
    o.head.e_shentsize = sizeof(o.sections[0]);
    o.head.e_shnum = sizeof(o.sections) / sizeof(o.sections[0]);
    o.head.e_shoff = sizeof(o.head);
    o.head.e_shstrndx = 1;

    // initialize stringtable.
    char* dst = stringtable;
    *dst = 0;
    dst++;
    auto add_string = [&dst, &stringtable](std::string_view a) -> Elf32_Word {
        const auto offset = dst - stringtable;
        dst += a.copy(dst, 256u - offset);
        *dst++ = '\0';
        return static_cast<Elf32_Word>(offset);
    };

    // create symbols
    o.symbs[2].st_name = add_string(gDataName); // ?BinaryDataForReadOnlyFileSystem@@3PADA");
    o.symbs[2].st_value = sizeof(uint64_t);
    o.symbs[2].st_size = static_cast<Elf32_Word>(size_of_data);
    o.symbs[2].st_info = o.symbs[1].st_info = ELF_ST_INFO(STB_GLOBAL, STT_OBJECT);
    o.symbs[2].st_other = o.symbs[1].st_other = STV_HIDDEN;
    o.symbs[2].st_shndx = o.symbs[1].st_shndx = 3;

    o.symbs[1].st_name = add_string(gSizeName);
    o.symbs[1].st_value = 0;
    o.symbs[1].st_size = sizeof(uint64_t);

    o.sections[2].sh_name = add_string(".symtab");
    o.sections[2].sh_type = SHT_SYMTAB;
    o.sections[2].sh_offset = offsetof(decltype(o), symbs); // sizeof(o) + size_of_section + stringtable_size;
    o.sections[2].sh_addralign = 8;
    o.sections[2].sh_size = sizeof(o.symbs);
    o.sections[2].sh_entsize = sizeof(o.symbs[0]);
    o.sections[2].sh_link = 1;
    o.sections[2].sh_info = 1; // index of first non-local symbol.

    std::string tmp = ".rodata.";
    tmp += secname;
    const char* sec;
    sec = tmp.c_str();
    /*sec = ".rodata.rofs"*/
    o.sections[3].sh_name = add_string(sec);
    o.sections[3].sh_type = SHT_PROGBITS;
    o.sections[3].sh_flags = SHF_ALLOC | SHF_MERGE;
    o.sections[3].sh_offset = sizeof(o);
    o.sections[3].sh_addralign = 8;
    o.sections[3].sh_size = static_cast<Elf32_Word>(size_of_section);

    o.sections[1].sh_name = add_string(".strtab");
    o.sections[1].sh_type = SHT_STRTAB;
    o.sections[1].sh_offset = static_cast<Elf32_Off>(sizeof(o) + size_of_section);
    o.sections[1].sh_addralign = 1;
    o.sections[1].sh_size = static_cast<Elf32_Word>(dst - stringtable);

    FILE* e = fopen(fname.c_str(), "wb");
    if (e == NULL) {
        printf("Could not open %s.\n", fname.c_str());
        exit(-1);
    }
    fwrite(&o, sizeof(o), 1, e);
    fwrite(data, size_of_section, 1, e);
    fwrite(stringtable, (size_t)o.sections[1].sh_size, 1, e);
    fclose(e);
}

void write_macho(const std::string& fname, const std::string& secname, size_t size_of_data_, const void* data)
{
#ifdef __APPLE__
    // Write fat header for X64_64 and ARM64 object
    struct fat_header fathdr;
    fathdr.magic = FAT_CIGAM;
    fathdr.nfat_arch = htonl(2); // big-endian values in fat header

    struct fat_arch archs[2] = {
        {
            htonl(CPU_TYPE_X86_64),
            htonl(CPU_SUBTYPE_X86_64_ALL),
            0,
            0, // size of data TBD
            htonl(3)
        },
        {
            htonl(CPU_TYPE_ARM64),
            htonl(CPU_SUBTYPE_ARM64_ALL),
            0, // offset,
            0, // size of data TBD
            htonl(3)
        },
    };

    size_t fpos = 0; // "file" offsets are actually relative to the architecture header
    archs[0].offset = htonl(sizeof(fathdr) + sizeof(archs));

    size_t size_of_section = size_of_data_ + sizeof(uint64_t);

    struct mach_header_64 x64_header = {
        MH_MAGIC_64,
        CPU_TYPE_X86_64,
        CPU_SUBTYPE_X86_64_ALL,
        MH_OBJECT,
        2, // ncmds
        sizeof(segment_command_64) + sizeof(section_64) + sizeof(symtab_command), // sizeofcmds
        0, // flags
        0  // reserved
    };

    struct mach_header_64 arm64_header = {
        MH_MAGIC_64,
        CPU_TYPE_ARM64,
        CPU_SUBTYPE_ARM64_ALL,
        MH_OBJECT,
        2, // ncmds
        sizeof(segment_command_64) + sizeof(section_64) + sizeof(symtab_command), // sizeofcmds
        0, // flags
        0  // reserved
    };

    struct segment_command_64 data_seg = {
        LC_SEGMENT_64,
        sizeof(data_seg) + sizeof(section_64),
        "", // for object files name is empty
        0, // vmaddress
        (size_of_section + 7) & ~7, // vmsize aligned to 8 bytes
        0, // fileoffset = TBD
        size_of_section, // filesize = TBD
        VM_PROT_READ, // maxprot
        VM_PROT_READ, // initprot
        1, // nsects
        SG_NORELOC  // flags
    };

    struct section_64 data_sect = {
        "__const",
        "__DATA",
        0, // addr
        size_of_section, // vmsize aligned to 8 bytes
        0, // offset = TBD
        3, // alignment = 2^3 = 8 bytes
        0, // reloff
        0, // nreloc
        S_REGULAR, // flags
        0, // reserved1
        0, // reserved2
    };

    std::string dataName = "_";
    dataName += gDataName;
    std::string sizeName = "_";
    sizeName += gSizeName;

    uint32_t string_size = dataName.size() + sizeName.size() + 3; // prepending plus two terminating nulls

    struct symtab_command symtab = {
        LC_SYMTAB,
        sizeof(symtab),
        0, // symoff = TBD
        2, // nsyms
        0, // stroff = TBD
        string_size, // strsize
    };

    fpos += sizeof(x64_header) + sizeof(data_seg) + sizeof(data_sect) + sizeof(symtab);

    data_seg.fileoff = static_cast<uint32_t>(fpos);
    data_sect.offset = static_cast<uint32_t>(fpos);
    fpos += size_of_section;

    size_t sectionAligned = (fpos + 7) & ~7;
    uint32_t sectionAlign = sectionAligned - fpos;
    fpos = sectionAligned;

    symtab.symoff = static_cast<uint32_t>(fpos);

    struct nlist_64 syms[2] = {
    {
        1, // first string
        N_EXT | N_SECT,
        1, // segment
        REFERENCE_FLAG_DEFINED,
        0
    },
    {
        static_cast<uint32_t>(sizeName.size() + 2), // second string
        N_EXT | N_SECT,
        1, // segment
        REFERENCE_FLAG_DEFINED,
        8
    }
    };

    fpos += sizeof(syms);

    symtab.stroff = static_cast<uint32_t>(fpos);
    fpos += string_size;

    archs[0].size = htonl(fpos);

    size_t aligned = (fpos + 7) & ~7;
    uint32_t align = aligned - fpos;
    archs[1].offset = htonl(aligned + sizeof(fathdr) + sizeof(archs));
    archs[1].size = archs[0].size;

    FILE* e = fopen(fname.c_str(), "wb");
    if (e == NULL) {
        printf("Could not open %s.\n", fname.c_str());
        exit(-1);
    }

    fwrite(&fathdr, sizeof(fathdr), 1, e);
    fwrite(&archs, sizeof(archs), 1, e);
    fwrite(&x64_header, sizeof(x64_header), 1, e);
    fwrite(&data_seg, sizeof(data_seg), 1, e);
    fwrite(&data_sect, sizeof(data_sect), 1, e);
    fwrite(&symtab, sizeof(symtab), 1, e);
    fwrite(data, size_of_section, 1, e);
    for(int i = 0; i < sectionAlign; i++) {
        fputc(0, e); // alignment byte to begin string table 8 byte boundary
    }
    fwrite(&syms, sizeof(syms), 1, e);
    fputc(0, e); // filler byte to begin string table as it starts indexing at 1
    fwrite(sizeName.c_str(), sizeName.size() + 1, 1, e);
    fwrite(dataName.c_str(), dataName.size() + 1, 1, e);

    for(int i = 0; i < align; i++) {
        fputc(0, e); // alignment byte to begin arm64 architecture at 8 byte boundary
    }

    fwrite(&arm64_header, sizeof(arm64_header), 1, e);
    fwrite(&data_seg, sizeof(data_seg), 1, e);
    fwrite(&data_sect, sizeof(data_sect), 1, e);
    fwrite(&symtab, sizeof(symtab), 1, e);
    fwrite(data, size_of_section, 1, e);
    for(int i = 0; i < sectionAlign; i++) {
        fputc(0, e); // alignment byte to begin string table 8 byte boundary
    }
    fwrite(&syms, sizeof(syms), 1, e);
    fputc(0, e); // filler byte to begin string table
    fwrite(sizeName.c_str(), sizeName.size() + 1, 1, e);
    fwrite(dataName.c_str(), dataName.size() + 1, 1, e);
    fclose(e);
#endif
}

int main(int argc, char* argv[])
{
    std::string obj32Name = "rofs_32.obj";
    std::string obj64Name = "rofs_64.obj";
    std::string o32Name = "rofs_32.o";
    std::string o64Name = "rofs_64.o";
    std::string x32Name = "rofs_x86.o";
    std::string x64Name = "rofs_x64.o";
    std::string macName = "rofs_mac.o";
    std::string secName = "rofs";

    bool buildX86 = true, buildX64 = true, buildV7 = true, buildV8 = true;
    bool buildWindows = true, buildAndroid = true, buildMac = true;

    std::string inPath = { "" }, roPath = { "" };

    int baseArg = 0;
    if (argc >= 2) {
        if (argv[1][0] == '-') {
            buildAndroid = false;
            buildWindows = false;
            buildMac = false;
            buildX86 = buildX64 = buildV7 = buildV8 = false;
        }
        bool platset = false;
        for (;;) {
            if (baseArg + 1 >= argc) {
                printf("Invalid argument!\n");
                return 0;
            }
            if (argv[baseArg + 1][0] != '-')
                break;
            if (strcmp(argv[baseArg + 1], "-linux") == 0) {
                platset = true;
                buildAndroid = true;
                baseArg++;
            } else if (strcmp(argv[baseArg + 1], "-android") == 0) {
                platset = true;
                buildAndroid = true;
                baseArg++;
            } else if (strcmp(argv[baseArg + 1], "-windows") == 0) {
                platset = true;
                buildWindows = true;
                baseArg++;
            } else if (strcmp(argv[baseArg + 1], "-mac") == 0) {
                platset = true;
                buildMac = true;
                baseArg++;
            } else if (strcmp(argv[baseArg + 1], "-x86") == 0) {
                buildX86 = true;
                baseArg++;
            } else if (strcmp(argv[baseArg + 1], "-x86_64") == 0) {
                buildAndroid = true;
                buildX64 = true;
                baseArg++;
            } else if (strcmp(argv[baseArg + 1], "-x64") == 0) {
                buildX64 = true;
                buildWindows = true;
                baseArg++;
            } else if (strcmp(argv[baseArg + 1], "-armeabi-v7a") == 0) {
                buildV7 = true;
                buildAndroid = true;
                platset = true;
                baseArg++;
            } else if (strcmp(argv[baseArg + 1], "-arm64-v8a") == 0) {
                buildV8 = true;
                buildAndroid = true;
                platset = true;
                baseArg++;
            } else if (strcmp(argv[baseArg + 1], "-extensions") == 0) {
                baseArg++;
                if (baseArg + 1 >= argc) {
                    printf("Invalid argument!\n");
                    return 0;
                }
                auto exts = std::string_view(argv[baseArg + 1]);
                while (!exts.empty()) {
                    auto pos = exts.find(';');
                    pos = std::min(pos, exts.size());
                    valid_exts.push_back(exts.substr(0, pos));
                    exts.remove_prefix(std::min(pos + 1, exts.size()));
                }
                baseArg++;
            } else {
                printf("Invalid argument!\n");
                return 0;
            }
        }
        if (!platset) {
            buildWindows = true;
            buildAndroid = true;
            buildMac = true;
        }

        if (argc < baseArg + 3) {
            printf("invalid args");
            return 0;
        }

        inPath = argv[baseArg + 1];
    } else {
        printf("Not enough arguments!\n");
        return 0;
    }
    if (argc >= baseArg + 3) {
        if (argv[baseArg + 2][0] == '/') {
            roPath = argv[baseArg + 2] + 1;
        } else {
            roPath = argv[baseArg + 2];
        }
    }
    if (argc >= baseArg + 5) {
        gDataName = argv[baseArg + 3];
        gSizeName = argv[baseArg + 4];
    }
    if (argc == baseArg + 6) {
        secName = obj32Name = obj64Name = o32Name = o64Name = x32Name = x64Name = macName = argv[baseArg + 5];
        obj32Name += "_32.obj";
        obj64Name += "_64.obj";
        o32Name += "_32.o";
        o64Name += "_64.o";
        x32Name += "_x86.o";
        x64Name += "_x64.o";
        macName += "_mac.o";
    }
    add_directory(inPath, roPath);

    // fix offsets
    size_t base_offset = sizeof(fs_entry) * (directory.size() + 1);
    for (auto& d : directory) {
        d.offset += base_offset;
    }

    // add terminator
    directory.push_back({ { 0 }, 0, 0 });

    const size_t size_of_dir = (sizeof(fs_entry) * directory.size());
    const size_t size_of_data = bin.size() + size_of_dir;
    uint8_t* data = new uint8_t[size_of_data + sizeof(uint64_t)];
    *(uint64_t*)(data) = size_of_data;
    memcpy(data + sizeof(uint64_t), directory.data(), size_of_dir);
    memcpy(data + sizeof(uint64_t) + size_of_dir, bin.data(), bin.size());

    // Build obj
    if (buildWindows) {
        if (buildX86) {
            write_obj(obj32Name, secName, size_of_data, data, false);
        }
        if (buildX64) {
            write_obj(obj64Name, secName, size_of_data, data, true);
        }
    }
    // Create .elf (.o)
    if (buildAndroid) {
        struct {
            Elf32_Ehdr head{};
            Elf32_Shdr sections[4]{};
            Elf32_Sym symbs[3]{};
        } o32;
        struct {
            Elf64_Ehdr head{};
            Elf64_Shdr sections[4]{};
            Elf64_Sym symbs[3]{};
        } o64;
        if (buildV7) {
            write_elf(o32, EM_ARM, o32Name, secName, size_of_data, data);
        }
        if (buildV8) {
            write_elf(o64, EM_AARCH64, o64Name, secName, size_of_data, data);
        }
        if (buildX86) {
            write_elf(o32, EM_386, x32Name, secName, size_of_data, data);
        }
        if (buildX64) {
            write_elf(o64, EM_X86_64, x64Name, secName, size_of_data, data);
        }
    }
    // Create mach-o (.o)
    if (buildMac) {
        write_macho(macName, secName, size_of_data, data);
    }
}
