/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "os/elf/process_elf.h"

#include <cstdio>
#include <cstring>

#include <base/containers/unique_ptr.h>
#include <core/plugin/intf_plugin.h>

#include "elf64.h"

CORE_BEGIN_NAMESPACE()
namespace {
enum FIND_TYPE { FIND_LOCALS = 0, FIND_GLOBALS = 1, FIND_ALL = 2 };

Elf64_Sym FindSymbolByName(const IFile::Ptr& f, const Elf64_Shdr& strTable, const Elf64_Shdr* symTable,
    const char* name, FIND_TYPE types /* 0 == local, 1=global, 2=all*/)
{
    if ((symTable == nullptr) || ((symTable->sh_type != SHT_SYMTAB) && (symTable->sh_type != SHT_DYNSYM))) {
        // no symtable given.
        return {};
    }
    if ((strTable.sh_type != SHT_STRTAB)) {
        // File has no strings? or stringtable link points to a non stringtable.
        return {};
    }

    Elf64_Xword startIndex = 0;
    Elf64_Xword endIndex = symTable->sh_size / sizeof(Elf64_Sym);

    if (types == FIND_LOCALS) {
        // only locals.. sh_info is the index for the first non-local symbol
        endIndex = symTable->sh_info;
    }
    if (types == FIND_GLOBALS) {
        // only globals ..
        startIndex = symTable->sh_info;
    }
    BASE_NS::vector<Elf64_Sym> symList(endIndex - startIndex);

    f->Seek(symTable->sh_offset + (startIndex * sizeof(Elf64_Sym)));
    f->Read(symList.data(), symList.size_in_bytes());
    for (const auto& sym : symList) {
        if (!sym.st_size) {
            continue;
        }
        f->Seek(strTable.sh_offset + sym.st_name);

        BASE_NS::string symbolName;
        char c;
        while (f->Read(&c, sizeof(c)) != 0 && c != '\0') {
            symbolName += c;
        }
        if (symbolName == name) {
            return sym;
        }
    }
    return {};
}

bool AddrToFileOffset(const BASE_NS::vector<Elf64_Shdr>& sections, uint64_t addr, uint64_t& fileOff)
{
    for (const auto& s : sections) {
        const uint64_t a0 = s.sh_addr;
        const uint64_t a1 = a0 + s.sh_size;
        if (addr >= a0 && addr < a1) {
            fileOff = s.sh_offset + (addr - a0);
            return true;
        }
    }
    return false;
}

bool ReadElfHeader(const IFile::Ptr& filePtr, Elf64_Ehdr& head)
{
    if (filePtr->Read(&head, sizeof(head)) != sizeof(head)) {
        return false;
    }
    return (std::memcmp(&head.e_ident, &Elf64_Ident, sizeof(ElfIdent)) == 0);
}

bool ReadSectionList(const IFile::Ptr& filePtr, const Elf64_Ehdr& head, BASE_NS::vector<Elf64_Shdr>& sectionList)
{
    if (!filePtr->Seek(head.e_shoff)) {
        return false;
    }
    return (filePtr->Read(sectionList.data(), sectionList.size_in_bytes()) == sectionList.size_in_bytes());
}

void FindSymTableAndRels(const IFile::Ptr& filePtr, const BASE_NS::vector<Elf64_Shdr>& sectionList,
    const Elf64_Shdr& sectStrTable, const Elf64_Shdr*& symTable, const Elf64_Shdr* (&rels)[10], int32_t& relcnt)
{
    for (const Elf64_Shdr& sh : sectionList) {
        filePtr->Seek(sectStrTable.sh_offset + sh.sh_name);
        BASE_NS::string name;
        char c;
        while (filePtr->Read(&c, sizeof(c)) != 0 && c != '\0') {
            name += c;
        }

        if (sh.sh_type == SHT_DYNSYM) {
            symTable = &sh;
        } else if (sh.sh_type == SHT_RELA) {
            if (relcnt < 10) {
                rels[relcnt++] = &sh;
            }
        }
    }
}

bool ReadPlugin(const IFile::Ptr& filePtr, const BASE_NS::vector<Elf64_Shdr>& sectionList,
    const Elf64_Sym& pluginDataSymbol, CORE_NS::IPlugin& plugin)
{
    // Read the CORE_NS::IPlugin instance out of the file
    const Elf64_Shdr& dataSh = sectionList[pluginDataSymbol.st_shndx];
    const uint64_t vaddrInSection = pluginDataSymbol.st_value - dataSh.sh_addr;
    const uint64_t pluginFileOff = dataSh.sh_offset + vaddrInSection;

    if (!filePtr->Seek(pluginFileOff)) {
        return false;
    }
    return (filePtr->Read(&plugin, sizeof(plugin)) == sizeof(plugin));
}

bool ReadSymEntries(const IFile::Ptr& filePtr, const Elf64_Shdr* symTable, BASE_NS::vector<Elf64_Sym>& symEntries)
{
    if (!filePtr->Seek(symTable->sh_offset)) {
        return false;
    }
    return (filePtr->Read(symEntries.data(), symEntries.size_in_bytes()) == symEntries.size_in_bytes());
}

Elf64_Addr GetDepPtrFieldAddr(const Elf64_Sym& pluginDataSymbol, const CORE_NS::IPlugin& plugin)
{
    const ptrdiff_t offset = uintptr_t(&plugin.pluginDependencies) - uintptr_t(&plugin);
    return pluginDataSymbol.st_value + offset;
}

bool IsRelocationForSymTable(
    const BASE_NS::vector<Elf64_Shdr>& sectionList, const Elf64_Shdr* symTable, const Elf64_Shdr* relt)
{
    if (!relt || relt->sh_link >= sectionList.size()) {
        return false;
    }
    const Elf64_Shdr& relTgtSyms = sectionList[relt->sh_link];
    return &relTgtSyms == symTable;
}

bool ReadRelaEntries(const IFile::Ptr& filePtr, const Elf64_Shdr* relt, BASE_NS::vector<Elf64_Rela>& rela)
{
    if (!filePtr->Seek(relt->sh_offset)) {
        return false;
    }
    return (filePtr->Read(rela.data(), rela.size_in_bytes()) == rela.size_in_bytes());
}

bool ResolveDepTarget(const BASE_NS::vector<Elf64_Shdr>& sectionList, const BASE_NS::vector<Elf64_Sym>& symEntries,
    const Elf64_Rela& r, uint64_t& depVAddr, uint64_t& depFileOff)
{
    const uint32_t relType = ELF64_R_TYPE(r.r_info);
    const uint32_t symIndex = ELF64_R_SYM(r.r_info);

    depVAddr = 0;
    depFileOff = 0;
    bool foundTarget = false;

    if (relType == R_AARCH64_RELATIVE) {
        depVAddr = static_cast<uint64_t>(r.r_addend);
        foundTarget = AddrToFileOffset(sectionList, depVAddr, depFileOff);
    }

    if (!foundTarget && symIndex < symEntries.size()) {
        const Elf64_Sym& s = symEntries[symIndex];
        const uint64_t vaSym = static_cast<uint64_t>(s.st_value + r.r_addend);
        if (AddrToFileOffset(sectionList, vaSym, depFileOff)) {
            depVAddr = vaSym;
            foundTarget = true;
        }
    }

    if (!foundTarget) {
        const uint64_t vaAdd = static_cast<uint64_t>(r.r_addend);
        if (AddrToFileOffset(sectionList, vaAdd, depFileOff)) {
            depVAddr = vaAdd;
            foundTarget = true;
        }
    }

    return foundTarget;
}

bool TryReadDependenciesAtOffset(
    const IFile::Ptr& filePtr, uint64_t depFileOff, BASE_NS::vector<BASE_NS::Uid>& dependencies)
{
    if (!filePtr->Seek(depFileOff)) {
        return false;
    }
    return (filePtr->Read(dependencies.data(), dependencies.size_in_bytes()) == dependencies.size_in_bytes());
}

bool TryLoadDepsFromRelocSection(const IFile::Ptr& filePtr, const BASE_NS::vector<Elf64_Shdr>& sectionList,
    const BASE_NS::vector<Elf64_Sym>& symEntries, const Elf64_Shdr* relt, const Elf64_Addr depPtrFieldAddr,
    BASE_NS::vector<BASE_NS::Uid>& dependencies, bool& depsLoaded)
{
    depsLoaded = false;

    const Elf64_Xword relCount = relt->sh_size / sizeof(Elf64_Rela);
    if (relCount == 0) {
        return true;
    }

    BASE_NS::vector<Elf64_Rela> rela(relCount);
    if (!ReadRelaEntries(filePtr, relt, rela)) {
        return false;
    }

    for (const Elf64_Rela& r : rela) {
        const Elf64_Addr ptrFieldAddr = r.r_offset;
        if (ptrFieldAddr != depPtrFieldAddr) {
            continue;
        }

        uint64_t depVAddr = 0;
        uint64_t depFileOff = 0;
        if (!ResolveDepTarget(sectionList, symEntries, r, depVAddr, depFileOff)) {
            continue;
        }

        if (!TryReadDependenciesAtOffset(filePtr, depFileOff, dependencies)) {
            continue;
        }

        depsLoaded = true;
        break;
    }

    return true;
}

bool TryLoadDepsFromRelocations(const IFile::Ptr& filePtr, const BASE_NS::vector<Elf64_Shdr>& sectionList,
    const Elf64_Shdr* symTable, const Elf64_Sym& pluginDataSymbol, const CORE_NS::IPlugin& plugin,
    const Elf64_Shdr* const (&rels)[10], int32_t relcnt, BASE_NS::vector<BASE_NS::Uid>& dependencies, bool& depsLoaded)
{
    depsLoaded = false;

    if (relcnt <= 0) {
        return true;
    }

    // Read dynamic symbol table entries
    const size_t symCount = symTable->sh_size / sizeof(Elf64_Sym);
    BASE_NS::vector<Elf64_Sym> symEntries(symCount);
    if (!ReadSymEntries(filePtr, symTable, symEntries)) {
        return false;
    }

    const Elf64_Addr depPtrFieldAddr = GetDepPtrFieldAddr(pluginDataSymbol, plugin);

    for (int i = 0; i < relcnt && !depsLoaded; ++i) {
        const Elf64_Shdr* relt = rels[i];
        if (!IsRelocationForSymTable(sectionList, symTable, relt)) {
            continue;
        }
        if (!TryLoadDepsFromRelocSection(
                filePtr, sectionList, symEntries, relt, depPtrFieldAddr, dependencies, depsLoaded)) {
            return false;
        }
    }

    return true;
}

void TryLoadDepsFromPointer(const IFile::Ptr& filePtr, const BASE_NS::vector<Elf64_Shdr>& sectionList,
    const CORE_NS::IPlugin& plugin, BASE_NS::vector<BASE_NS::Uid>& dependencies, bool& depsLoaded)
{
    const uint64_t depVAddr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(plugin.pluginDependencies.data()));
    uint64_t depFileOff = 0;
    if (AddrToFileOffset(sectionList, depVAddr, depFileOff)) {
        if (filePtr->Seek(depFileOff) &&
            filePtr->Read(dependencies.data(), dependencies.size_in_bytes()) == dependencies.size_in_bytes()) {
            depsLoaded = true;
        }
    }
}

bool LoadDependenciesFromFile(const IFile::Ptr& filePtr, const BASE_NS::vector<Elf64_Shdr>& sectionList,
    const Elf64_Shdr* symTable, const Elf64_Sym& pluginDataSymbol, const CORE_NS::IPlugin& plugin,
    const Elf64_Shdr* const (&rels)[10], int32_t relcnt, BASE_NS::vector<BASE_NS::Uid>& dependencies, bool& depsLoaded)
{
    depsLoaded = false;

    if (!TryLoadDepsFromRelocations(
            filePtr, sectionList, symTable, pluginDataSymbol, plugin, rels, relcnt, dependencies, depsLoaded)) {
        return false;
    }

    if (!depsLoaded) {
        TryLoadDepsFromPointer(filePtr, sectionList, plugin, dependencies, depsLoaded);
    }

    return true;
}

bool ReadElfSectionsValidated(const IFile::Ptr& filePtr, Elf64_Ehdr& head, BASE_NS::vector<Elf64_Shdr>& sectionList)
{
    if (!ReadElfHeader(filePtr, head) || head.e_shoff == 0 || head.e_shnum == 0) {
        return false;
    }
    sectionList = BASE_NS::vector<Elf64_Shdr>(head.e_shnum);
    return ReadSectionList(filePtr, head, sectionList) && (head.e_shstrndx < sectionList.size());
}

bool FindPluginDataSymbolInElf(const IFile::Ptr& filePtr, const Elf64_Ehdr& head,
    const BASE_NS::vector<Elf64_Shdr>& sectionList, const Elf64_Shdr*& symTable, const Elf64_Shdr* (&rels)[10],
    int32_t& relcnt, Elf64_Sym& pluginDataSymbol)
{
    const Elf64_Shdr& sectStrTable = sectionList[head.e_shstrndx];
    FindSymTableAndRels(filePtr, sectionList, sectStrTable, symTable, rels, relcnt);
    if (!symTable || symTable->sh_link >= sectionList.size()) {
        return false;
    }

    const Elf64_Shdr& dynStrTable = sectionList[symTable->sh_link];
    pluginDataSymbol = FindSymbolByName(filePtr, dynStrTable, symTable, "gPluginData", FIND_GLOBALS);
    if (!pluginDataSymbol.st_size || pluginDataSymbol.st_shndx >= sectionList.size()) {
        return false;
    }

    return true;
}
} // namespace

PluginData ProcessElf(const BASE_NS::string_view filePath, const IFile::Ptr& filePtr)
{
    if (!filePtr) {
        return {};
    }
    Elf64_Ehdr head {};
    BASE_NS::vector<Elf64_Shdr> sectionList;
    if (!ReadElfSectionsValidated(filePtr, head, sectionList)) {
        return {};
    }
    const Elf64_Shdr* symTable = nullptr;
    int32_t relcnt = 0;
    const Elf64_Shdr* rels[10] = { nullptr };
    Elf64_Sym pluginDataSymbol {};
    if (!FindPluginDataSymbolInElf(filePtr, head, sectionList, symTable, rels, relcnt, pluginDataSymbol)) {
        return {};
    }

    CORE_NS::IPlugin plugin {};
    if (!ReadPlugin(filePtr, sectionList, pluginDataSymbol, plugin)) {
        return {};
    }

    PluginData result;
    result.filename = BASE_NS::string(filePath);
    result.pluginUid = plugin.version.uid;

    const size_t depCount = plugin.pluginDependencies.size();
    if (depCount == 0) {
        // No dependencies
        result.dependencies.clear();
        return result;
    }

    BASE_NS::vector<BASE_NS::Uid> dependencies(depCount);
    bool depsLoaded = false;
    if (!LoadDependenciesFromFile(
            filePtr, sectionList, symTable, pluginDataSymbol, plugin, rels, relcnt, dependencies, depsLoaded)) {
        return {};
    }

    if (depsLoaded) {
        result.dependencies = std::move(dependencies);
    } else {
        // If everything fails, better to report no deps than random garbage
        result.dependencies.clear();
    }
    return result;
}

CORE_END_NAMESPACE()
