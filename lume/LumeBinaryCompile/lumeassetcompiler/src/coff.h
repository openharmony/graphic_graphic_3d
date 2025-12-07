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

#ifndef LUME_COFF_H
#define LUME_COFF_H

#include <cstdint>

#define IMAGE_FILE_MACHINE_UNKNOWN 0
#define IMAGE_FILE_MACHINE_I386 0x014c
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x40
#define IMAGE_SCN_ALIGN_4BYTES 0x300000
#define IMAGE_SCN_MEM_READ 0x40000000
#define IMAGE_SYM_TYPE_CHAR 0x2
#define IMAGE_SYM_DTYPE_ARRAY 0x3
#define IMAGE_SYM_CLASS_EXTERNAL 0x2
#pragma pack(push, 4)
struct IMAGE_FILE_HEADER {
    uint16_t machine;
    uint16_t numberOfSections;
    uint32_t timeDateStamp;
    uint32_t pointerToSymbolTable;
    uint32_t numberOfSymbols;
    uint16_t sizeOfOptionalHeader;
    uint16_t characteristics;
};

struct IMAGE_SECTION_HEADER {
    uint8_t name[8];
    union {
        uint32_t physicalAddress;
        uint32_t virtualSize;
    } misc;
    uint32_t virtualAddress;
    uint32_t sizeOfRawData;
    uint32_t pointerToRawData;
    uint32_t pointerToRelocations;
    uint32_t pointerToLinenumbers;
    uint16_t numberOfRelocations;
    uint16_t numberOfLinenumbers;
    uint32_t characteristics;
};

#pragma pack(pop)
#pragma pack(push, 2)
struct IMAGE_SYMBOL {
    union {
        uint8_t shortName[8];
        struct {
            uint32_t Short;
            uint32_t Long;
        } name;
        uint32_t longName[2];
    } n;

    uint32_t value;
    int16_t sectionNumber;
    uint16_t type;
    uint8_t storageClass;
    uint8_t numberOfAuxSymbols;
};
#pragma pack(pop)
#endif // LUME_COFF_H
