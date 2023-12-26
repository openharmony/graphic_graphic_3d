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
#ifndef __LUME_ELF_DEFINES__
#define __LUME_ELF_DEFINES__
#include <cstdint>
#define EI_NIDENT 16
#define ET_REL 1
#define EM_NONE 0

#define EV_CURRENT 1 /*original format...*/
#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELFDATA2MSB 1
#define ELFOSABI_NONE 0

#define EM_NONE 0
#define EM_386 3
#define EM_ARM 40      /* ARM 32 bit */
#define EM_X86_64 62   /* AMD x86-64 */
#define EM_AARCH64 183 /* ARM 64 bit */

#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3

#define SHF_ALLOC 0x2
#define SHF_MERGE 0x10
#define SHN_UNDEF 0
#define STB_GLOBAL 1
#define STT_OBJECT 1
#define STV_HIDDEN 2
#define ELF_ST_BIND(info) ((info) >> 4)
#define ELF_ST_TYPE(info) ((info)&0xf)
#define ELF_ST_INFO(bind, type) (((bind) << 4) + ((type)&0xf))

typedef struct ElfIdent {
    char EI_MAG0;
    char EI_MAG1;
    char EI_MAG2;
    char EI_MAG3;
    uint8_t EI_CLASS;
    uint8_t EI_DATA;
    uint8_t EI_VERSION;
    uint8_t EI_OSABI;
    uint8_t EI_PAD[EI_NIDENT - 8];
} ElfIdent;

#endif