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

#ifndef LUME_ELF_64_H
#define LUME_ELF_64_H
#include "elf_common.h"

#define ELF64_ST_VISIBILITY(o) ((o)&0x3)

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef uint64_t Elf64_Xword;

struct Elf64_Ehdr {
    ElfIdent ident = { 0x7f, 'E', 'L', 'F', ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE };
    Elf64_Half type;
    Elf64_Half machine;
    Elf64_Word version;
    Elf64_Addr entry;
    Elf64_Off phoff;
    Elf64_Off shoff;
    Elf64_Word flags;
    Elf64_Half ehsize;
    Elf64_Half phentsize;
    Elf64_Half phnum;
    Elf64_Half shentsize;
    Elf64_Half shnum;
    Elf64_Half shstrndx;
};

struct Elf64_Shdr {
    Elf64_Word name;
    Elf64_Word type;
    Elf64_Xword flags;
    Elf64_Addr addr;
    Elf64_Off offset;
    Elf64_Xword size;
    Elf64_Word link;
    Elf64_Word info;
    Elf64_Xword addralign;
    Elf64_Xword entsize;
};

struct Elf64_Sym {
    Elf64_Word name;
    unsigned char info;
    unsigned char other;
    Elf64_Half shndx;
    Elf64_Addr value;
    Elf64_Xword size;
};

#endif // LUME_ELF_64_H
