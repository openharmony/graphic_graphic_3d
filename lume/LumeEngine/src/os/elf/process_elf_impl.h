/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef LUME_OS_PROCESS_ELF_IMPL_H
#define LUME_OS_PROCESS_ELF_IMPL_H

#include <cstdio>
#include <cstring>

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/plugin/intf_plugin.h>

#include "elf32.h"
#include "elf64.h"
#include "os/elf/process_elf.h"
#include "os/intf_library.h"

CORE_BEGIN_NAMESPACE()
namespace ElfDetail {

constexpr uint64_t MAX_ELF_SYMBOL_COUNT = 1000000U;
constexpr uint64_t MAX_ELF_RELOCATION_COUNT = 1000000U;
constexpr uint16_t MAX_ELF_SECTION_COUNT = 10000U;
constexpr size_t MAX_ELF_NAME_LENGTH = 4096U;

enum FIND_TYPE { FIND_LOCALS = 0, FIND_GLOBALS = 1, FIND_ALL = 2 };

struct Elf64Traits {
    using Addr = Elf64_Addr;
    using Off = Elf64_Off;
    using Half = Elf64_Half;
    using Word = Elf64_Word;
    using XWord = Elf64_Xword;
    using Ehdr = Elf64_Ehdr;
    using Shdr = Elf64_Shdr;
    using Sym = Elf64_Sym;
    using Rela = Elf64_Rela;
    static const ElfIdent& Ident()
    {
        return Elf64_Ident;
    }
    static constexpr uint32_t RELATIVE = R_AARCH64_RELATIVE;
    static constexpr uint32_t R_SYM(XWord i)
    {
        return static_cast<uint32_t>(ELF64_R_SYM(i));
    }
    static constexpr uint32_t R_TYPE(XWord i)
    {
        return static_cast<uint32_t>(ELF64_R_TYPE(i));
    }
};

struct Elf32Traits {
    using Addr = Elf32_Addr;
    using Off = Elf32_Off;
    using Half = Elf32_Half;
    using Word = Elf32_Word;
    using XWord = Elf32_Word;
    using Ehdr = Elf32_Ehdr;
    using Shdr = Elf32_Shdr;
    using Sym = Elf32_Sym;
    using Rela = Elf32_Rela;
    static const ElfIdent& Ident()
    {
        return Elf32_Ident;
    }
    static constexpr uint32_t RELATIVE = R_ARM_RELATIVE;
    static constexpr uint32_t R_SYM(Word i)
    {
        return ELF32_R_SYM(i);
    }
    static constexpr uint32_t R_TYPE(Word i)
    {
        return ELF32_R_TYPE(i);
    }
};

template <typename T>
typename T::Sym FindSymbolByName(const IFile::Ptr& f, const typename T::Shdr& strTable,
    const typename T::Shdr* symTable, const char* name, FIND_TYPE types)
{
    using Sym = typename T::Sym;
    using XWord = typename T::XWord;

    if ((symTable == nullptr) || ((symTable->sh_type != SHT_SYMTAB) && (symTable->sh_type != SHT_DYNSYM))) {
        return {};
    }
    if ((strTable.sh_type != SHT_STRTAB)) {
        return {};
    }

    XWord startIndex = 0;
    XWord endIndex = symTable->sh_size / sizeof(Sym);
    if (endIndex > MAX_ELF_SYMBOL_COUNT) {
        return {};
    }

    if (types == FIND_LOCALS) {
        endIndex = symTable->sh_info;
    }
    if (types == FIND_GLOBALS) {
        startIndex = symTable->sh_info;
    }
    if (startIndex > endIndex) {
        return {};
    }
    BASE_NS::vector<Sym> symList(endIndex - startIndex);

    if (!f->Seek(symTable->sh_offset + (startIndex * sizeof(Sym)))) {
        return {};
    }
    if (f->Read(symList.data(), symList.size_in_bytes()) != symList.size_in_bytes()) {
        return {};
    }
    for (const auto& sym : symList) {
        if (!sym.st_size) {
            continue;
        }
        if (sym.st_name >= strTable.sh_size) {
            continue;
        }
        if (!f->Seek(strTable.sh_offset + sym.st_name)) {
            continue;
        }

        BASE_NS::string symbolName;
        char c;
        while (f->Read(&c, sizeof(c)) != 0 && c != '\0') {
            symbolName += c;
            if (symbolName.size() > MAX_ELF_NAME_LENGTH) {
                break;
            }
        }
        if (symbolName == name) {
            return sym;
        }
    }
    return {};
}

template <typename T>
bool AddrToFileOffset(const BASE_NS::vector<typename T::Shdr>& sections, uint64_t addr, uint64_t& fileOff)
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

template <typename T>
bool ReadElfHeader(const IFile::Ptr& filePtr, typename T::Ehdr& head)
{
    if (filePtr->Read(&head, sizeof(head)) != sizeof(head)) {
        return false;
    }
    return (std::memcmp(&head.e_ident, &T::Ident(), sizeof(ElfIdent)) == 0);
}

template <typename T>
bool ReadSectionList(
    const IFile::Ptr& filePtr, const typename T::Ehdr& head, BASE_NS::vector<typename T::Shdr>& sectionList)
{
    if (!filePtr->Seek(head.e_shoff)) {
        return false;
    }
    return (filePtr->Read(sectionList.data(), sectionList.size_in_bytes()) == sectionList.size_in_bytes());
}

template <typename T>
void FindSymTableAndRels(const IFile::Ptr& filePtr, const BASE_NS::vector<typename T::Shdr>& sectionList,
    const typename T::Shdr& sectStrTable, const typename T::Shdr*& symTable, const typename T::Shdr* (&rels)[10],
    int32_t& relcnt)
{
    using Shdr = typename T::Shdr;
    for (const Shdr& sh : sectionList) {
        filePtr->Seek(sectStrTable.sh_offset + sh.sh_name);
        BASE_NS::string name;
        char c;
        while (filePtr->Read(&c, sizeof(c)) != 0 && c != '\0') {
            name += c;
            if (name.size() > MAX_ELF_NAME_LENGTH) {
                break;
            }
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

template <typename T>
bool ReadPlugin(const IFile::Ptr& filePtr, const BASE_NS::vector<typename T::Shdr>& sectionList,
    const typename T::Sym& pluginDataSymbol, CORE_NS::IPlugin& plugin)
{
    const typename T::Shdr& dataSh = sectionList[pluginDataSymbol.st_shndx];
    const uint64_t vaddrInSection = pluginDataSymbol.st_value - dataSh.sh_addr;
    const uint64_t pluginFileOff = dataSh.sh_offset + vaddrInSection;

    if (!filePtr->Seek(pluginFileOff)) {
        return false;
    }
    return (filePtr->Read(&plugin, sizeof(plugin)) == sizeof(plugin));
}

template <typename T>
bool ReadSymEntries(
    const IFile::Ptr& filePtr, const typename T::Shdr* symTable, BASE_NS::vector<typename T::Sym>& symEntries)
{
    if (!filePtr->Seek(symTable->sh_offset)) {
        return false;
    }
    return (filePtr->Read(symEntries.data(), symEntries.size_in_bytes()) == symEntries.size_in_bytes());
}

template <typename T>
typename T::Addr GetDepPtrFieldAddr(const typename T::Sym& pluginDataSymbol, const CORE_NS::IPlugin& plugin)
{
    const ptrdiff_t offset = ptrdiff_t(uintptr_t(&plugin.pluginDependencies) - uintptr_t(&plugin));
    return static_cast<typename T::Addr>(pluginDataSymbol.st_value + static_cast<typename T::Addr>(offset));
}

template <typename T>
bool IsRelocationForSymTable(const BASE_NS::vector<typename T::Shdr>& sectionList, const typename T::Shdr* symTable,
    const typename T::Shdr* relt)
{
    if (!relt || relt->sh_link >= sectionList.size()) {
        return false;
    }
    const typename T::Shdr& relTgtSyms = sectionList[relt->sh_link];
    return &relTgtSyms == symTable;
}

template <typename T>
bool ReadRelaEntries(const IFile::Ptr& filePtr, const typename T::Shdr* relt, BASE_NS::vector<typename T::Rela>& rela)
{
    if (!filePtr->Seek(relt->sh_offset)) {
        return false;
    }
    return (filePtr->Read(rela.data(), rela.size_in_bytes()) == rela.size_in_bytes());
}

template <typename T>
bool ResolveDepTarget(const BASE_NS::vector<typename T::Shdr>& sectionList,
    const BASE_NS::vector<typename T::Sym>& symEntries, const typename T::Rela& r, uint64_t& depVAddr,
    uint64_t& depFileOff)
{
    const uint32_t relType = T::R_TYPE(r.r_info);
    const uint32_t symIndex = T::R_SYM(r.r_info);

    depVAddr = 0;
    depFileOff = 0;
    bool foundTarget = false;

    if (relType == T::RELATIVE) {
        depVAddr = static_cast<uint64_t>(r.r_addend);
        foundTarget = AddrToFileOffset<T>(sectionList, depVAddr, depFileOff);
    }

    if (!foundTarget && symIndex < symEntries.size()) {
        const typename T::Sym& s = symEntries[symIndex];
        const uint64_t vaSym = static_cast<uint64_t>(s.st_value + r.r_addend);
        if (AddrToFileOffset<T>(sectionList, vaSym, depFileOff)) {
            depVAddr = vaSym;
            foundTarget = true;
        }
    }

    if (!foundTarget) {
        const uint64_t vaAdd = static_cast<uint64_t>(r.r_addend);
        if (AddrToFileOffset<T>(sectionList, vaAdd, depFileOff)) {
            depVAddr = vaAdd;
            foundTarget = true;
        }
    }

    return foundTarget;
}

template <typename T>
bool TryReadDependenciesAtOffset(
    const IFile::Ptr& filePtr, uint64_t depFileOff, BASE_NS::vector<BASE_NS::Uid>& dependencies)
{
    if (!filePtr->Seek(depFileOff)) {
        return false;
    }
    return (filePtr->Read(dependencies.data(), dependencies.size_in_bytes()) == dependencies.size_in_bytes());
}

template <typename T>
bool TryLoadDepsFromRelocSection(const IFile::Ptr& filePtr, const BASE_NS::vector<typename T::Shdr>& sectionList,
    const BASE_NS::vector<typename T::Sym>& symEntries, const typename T::Shdr* relt,
    const typename T::Addr depPtrFieldAddr, BASE_NS::vector<BASE_NS::Uid>& dependencies, bool& depsLoaded)
{
    using Rela = typename T::Rela;
    using Addr = typename T::Addr;

    depsLoaded = false;

    const uint64_t relCount = relt->sh_size / sizeof(Rela);
    if (relCount == 0) {
        return true;
    }
    if (relCount > MAX_ELF_RELOCATION_COUNT) {
        return false;
    }

    BASE_NS::vector<Rela> rela(relCount);
    if (!ReadRelaEntries<T>(filePtr, relt, rela)) {
        return false;
    }

    for (const Rela& r : rela) {
        const Addr ptrFieldAddr = r.r_offset;
        if (ptrFieldAddr != depPtrFieldAddr) {
            continue;
        }

        uint64_t depVAddr = 0;
        uint64_t depFileOff = 0;
        if (!ResolveDepTarget<T>(sectionList, symEntries, r, depVAddr, depFileOff)) {
            continue;
        }

        if (!TryReadDependenciesAtOffset<T>(filePtr, depFileOff, dependencies)) {
            continue;
        }

        depsLoaded = true;
        break;
    }

    return true;
}

template <typename T>
bool TryLoadDepsFromRelocations(const IFile::Ptr& filePtr, const BASE_NS::vector<typename T::Shdr>& sectionList,
    const typename T::Shdr* symTable, const typename T::Sym& pluginDataSymbol, const CORE_NS::IPlugin& plugin,
    const typename T::Shdr* const (&rels)[10], int32_t relcnt, BASE_NS::vector<BASE_NS::Uid>& dependencies,
    bool& depsLoaded)
{
    using Sym = typename T::Sym;
    using Shdr = typename T::Shdr;
    using Addr = typename T::Addr;

    depsLoaded = false;

    if (relcnt <= 0) {
        return true;
    }

    const size_t symCount = symTable->sh_size / sizeof(Sym);
    BASE_NS::vector<Sym> symEntries(symCount);
    if (!ReadSymEntries<T>(filePtr, symTable, symEntries)) {
        return false;
    }

    const Addr depPtrFieldAddr = GetDepPtrFieldAddr<T>(pluginDataSymbol, plugin);

    for (int i = 0; i < relcnt && !depsLoaded; ++i) {
        const Shdr* relt = rels[i];
        if (!IsRelocationForSymTable<T>(sectionList, symTable, relt)) {
            continue;
        }
        if (!TryLoadDepsFromRelocSection<T>(
                filePtr, sectionList, symEntries, relt, depPtrFieldAddr, dependencies, depsLoaded)) {
            return false;
        }
    }

    return true;
}

template <typename T>
void TryLoadDepsFromPointer(const IFile::Ptr& filePtr, const BASE_NS::vector<typename T::Shdr>& sectionList,
    const CORE_NS::IPlugin& plugin, BASE_NS::vector<BASE_NS::Uid>& dependencies, bool& depsLoaded)
{
    const uint64_t depVAddr = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(plugin.pluginDependencies.data()));
    uint64_t depFileOff = 0;
    if (AddrToFileOffset<T>(sectionList, depVAddr, depFileOff)) {
        if (filePtr->Seek(depFileOff) &&
            filePtr->Read(dependencies.data(), dependencies.size_in_bytes()) == dependencies.size_in_bytes()) {
            depsLoaded = true;
        }
    }
}

template <typename T>
bool LoadDependenciesFromFile(const IFile::Ptr& filePtr, const BASE_NS::vector<typename T::Shdr>& sectionList,
    const typename T::Shdr* symTable, const typename T::Sym& pluginDataSymbol, const CORE_NS::IPlugin& plugin,
    const typename T::Shdr* const (&rels)[10], int32_t relcnt, BASE_NS::vector<BASE_NS::Uid>& dependencies,
    bool& depsLoaded)
{
    depsLoaded = false;

    if (!TryLoadDepsFromRelocations<T>(
            filePtr, sectionList, symTable, pluginDataSymbol, plugin, rels, relcnt, dependencies, depsLoaded)) {
        return false;
    }

    if (!depsLoaded) {
        TryLoadDepsFromPointer<T>(filePtr, sectionList, plugin, dependencies, depsLoaded);
    }

    return true;
}

template <typename T>
bool ReadElfSectionsValidated(
    const IFile::Ptr& filePtr, typename T::Ehdr& head, BASE_NS::vector<typename T::Shdr>& sectionList)
{
    if (!ReadElfHeader<T>(filePtr, head) || head.e_shoff == 0 || head.e_shnum == 0) {
        return false;
    }
    if (head.e_shnum > MAX_ELF_SECTION_COUNT) {
        return false;
    }
    sectionList = BASE_NS::vector<typename T::Shdr>(head.e_shnum);
    return ReadSectionList<T>(filePtr, head, sectionList) && (head.e_shstrndx < sectionList.size());
}

template <typename T>
bool FindPluginDataSymbolInElf(const IFile::Ptr& filePtr, const typename T::Ehdr& head,
    const BASE_NS::vector<typename T::Shdr>& sectionList, const typename T::Shdr*& symTable,
    const typename T::Shdr* (&rels)[10], int32_t& relcnt, typename T::Sym& pluginDataSymbol)
{
    using Shdr = typename T::Shdr;

    const Shdr& sectStrTable = sectionList[head.e_shstrndx];
    FindSymTableAndRels<T>(filePtr, sectionList, sectStrTable, symTable, rels, relcnt);
    if (!symTable || symTable->sh_link >= sectionList.size()) {
        return false;
    }

    const Shdr& dynStrTable = sectionList[symTable->sh_link];
    pluginDataSymbol = FindSymbolByName<T>(filePtr, dynStrTable, symTable, "gPluginData", FIND_GLOBALS);
    if (!pluginDataSymbol.st_size || pluginDataSymbol.st_shndx >= sectionList.size()) {
        return false;
    }

    return true;
}

template <typename T>
PluginData ProcessElfImpl(const BASE_NS::string_view filePath, const IFile::Ptr& filePtr)
{
    using Ehdr = typename T::Ehdr;
    using Shdr = typename T::Shdr;
    using Sym = typename T::Sym;

    if (!filePtr) {
        return {};
    }
    Ehdr head{};
    BASE_NS::vector<Shdr> sectionList;
    if (!ReadElfSectionsValidated<T>(filePtr, head, sectionList)) {
        return {};
    }
    const Shdr* symTable = nullptr;
    int32_t relcnt = 0;
    const Shdr* rels[10] = {nullptr};
    Sym pluginDataSymbol{};
    if (!FindPluginDataSymbolInElf<T>(filePtr, head, sectionList, symTable, rels, relcnt, pluginDataSymbol)) {
        return {};
    }

    CORE_NS::IPlugin plugin{};
    if (!ReadPlugin<T>(filePtr, sectionList, pluginDataSymbol, plugin)) {
        return {};
    }

    if (plugin.typeUid != IPlugin::UID) {
        return {};
    }

    const size_t depCount = plugin.pluginDependencies.size();
    if (depCount > MAX_PLUGIN_DEPENDENCY_COUNT) {
        return {};
    }

    PluginData result;
    result.filename = BASE_NS::string(filePath);
    result.pluginUid = plugin.version.uid;
    if (depCount == 0) {
        result.dependencies.clear();
        return result;
    }

    BASE_NS::vector<BASE_NS::Uid> dependencies(depCount);
    bool depsLoaded = false;
    if (!LoadDependenciesFromFile<T>(
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

}  // namespace ElfDetail
CORE_END_NAMESPACE()

#endif  // LUME_OS_PROCESS_ELF_IMPL_H
