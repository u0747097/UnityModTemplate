#include "pch.h"
#include "mem.h"

std::unordered_map<void*, Mem::PatchInfo> Mem::m_patches;

void Mem::patch(void* address, const char* bytes, size_t len)
{
    auto it = m_patches.find(address);
    if (it == m_patches.end())
    {
        // If a patch doesn't exist, create a new one
        PatchInfo info;
        info.address = address;
        info.originalBytes.resize(len);
        memcpy(info.originalBytes.data(), address, len);
        // Store the patch info
        m_patches[address] = info;
    }

    DWORD oldProtect;
    VirtualProtect(address, len, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(address, bytes, len);
    VirtualProtect(address, len, oldProtect, &oldProtect);
}

void Mem::patch(uintptr_t address, const char* bytes)
{
    patch(reinterpret_cast<void*>(address), bytes, strlen(bytes));
}

void Mem::nop(uintptr_t address, const size_t len)
{
    std::vector<BYTE> nops(len, 0x90);
    patch(reinterpret_cast<void*>(address), reinterpret_cast<const char*>(nops.data()), len);
}

void Mem::restore(uintptr_t address)
{
    auto it = m_patches.find(reinterpret_cast<void*>(address));
    if (it != m_patches.end())
    {
        // Restore the original bytes
        const auto& info = it->second;
        DWORD oldProtect;
        VirtualProtect(info.address, info.originalBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(info.address, info.originalBytes.data(), info.originalBytes.size());
        VirtualProtect(info.address, info.originalBytes.size(), oldProtect, &oldProtect);

        // Remove the patch info
        m_patches.erase(it);
    }
}

void Mem::restore(std::initializer_list<uintptr_t> address)
{
    for (const auto addr : address) restore(addr);
}

uintptr_t Mem::patternScan(uintptr_t module, const char* signature)
{
    static auto patternToByte = [](const char* pattern)
    {
        auto bytes = std::vector<int>{};
        const auto start = const_cast<char*>(pattern);
        const auto end = const_cast<char*>(pattern) + strlen(pattern);

        for (auto current = start; current < end; ++current)
        {
            if (*current == '?')
            {
                ++current;
                if (*current == '?' && current < end) ++current;
                bytes.push_back(-1);
            }
            else
            {
                bytes.push_back(strtoul(current, &current, 16));
            }
        }
        return bytes;
    };

    const auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
    const auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint8_t*>(module) + dosHeader->
        e_lfanew);

    const auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
    auto patternBytes = patternToByte(signature);
    const auto scanBytes = reinterpret_cast<uint8_t*>(module);

    const auto s = patternBytes.size();
    const auto d = patternBytes.data();

    for (auto i = 0ul; i < sizeOfImage - s; ++i)
    {
        bool found = true;
        for (auto j = 0ul; j < s; ++j)
        {
            if (scanBytes[i + j] != d[j] && d[j] != -1)
            {
                found = false;
                break;
            }
        }
        if (found)
        {
            // return reinterpret_cast<uintptr_t>(&scanBytes[i]);
            // Return the address of the pattern
            return module + i;
        }
    }
    return 0;
}

void* Mem::allocateNearbyMemory(uintptr_t address, size_t size)
{
    // Get the system allocation granularity
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // Calculate the start and end addresses within 2GB range for x64
    constexpr uintptr_t maxDistance = 0x7FF00000; // ~2GB for relative jumps
    auto start = address > maxDistance ? address - maxDistance : 0;
    auto end = address + maxDistance;

    // Align to allocation granularity
    start = start / si.dwAllocationGranularity * si.dwAllocationGranularity;

    // Iterate through the memory range and find a suitable location
    MEMORY_BASIC_INFORMATION mbi;
    for (auto current = start; current < end; current += si.dwAllocationGranularity)
    {
        if (!VirtualQuery(reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi))) continue;

        // If the memory is free and large enough, allocate it
        if (mbi.State == MEM_FREE && mbi.RegionSize >= size)
        {
            auto ptr = VirtualAlloc(reinterpret_cast<LPVOID>(current), size, MEM_COMMIT | MEM_RESERVE,
                                    PAGE_EXECUTE_READWRITE);
            if (ptr) return ptr;
        }
    }

    // If we couldn't find a suitable location, just allocate the memory anywhere
    return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

void Mem::createTrampoline(uintptr_t address, void* destination, size_t length)
{
    // Check if the length is at least 14 bytes
    if (length < 14) return;

    // Create a byte array based on the length
    std::vector<BYTE> trampoline(length, 0x90);

    // Set the jmp [rip] opcode
    trampoline[0] = 0xFF;
    trampoline[1] = 0x25;
    memset(&trampoline[2], 0, 4);

    // Copy the destination address to the place holder of the trampoline
    memcpy(&trampoline[6], &destination, 8);

    auto it = m_patches.find(reinterpret_cast<void*>(address));
    if (it != m_patches.end())
    {
        // If a patch already exists, update the info with the new trampoline
        it->second.hasTrampoline = true;
        it->second.trampolineDestination = destination;
    }
    else
    {
        // If a patch doesn't exist, create a new one
        PatchInfo info;
        info.address = reinterpret_cast<void*>(address);
        info.originalBytes.resize(length);
        memcpy(info.originalBytes.data(), reinterpret_cast<void*>(address), length);
        info.hasTrampoline = true;
        info.trampolineDestination = destination;
        // Store the patch info
        m_patches[reinterpret_cast<void*>(address)] = info;
    }

    // Patch the original function with the trampoline
    patch(reinterpret_cast<void*>(address), reinterpret_cast<const char*>(trampoline.data()), trampoline.size());
}

void Mem::removeTrampoline(uintptr_t address)
{
    auto it = m_patches.find(reinterpret_cast<void*>(address));
    if (it != m_patches.end())
    {
        if (!it->second.hasTrampoline) return;

        // Free the allocated memory for the trampoline
        VirtualFree(it->second.trampolineDestination, 0, MEM_RELEASE);

        // Restore the original bytes
        restore(reinterpret_cast<uintptr_t>(it->second.address));
    }
}

void Mem::writeInstructions(void* destination, const BYTE instructions[], size_t instructionLen,
                            uintptr_t retAddress)
{
    // Calculate the length of the instructions plus the far jump
    auto length = instructionLen + 14;

    // Create a buffer to store the instructions and the far jump
    std::vector<BYTE> buffer(length, 0x00);

    // Copy the instructions to the buffer
    memcpy(buffer.data(), instructions, instructionLen);

    // Set the far jump
    buffer[instructionLen] = 0xFF;
    buffer[instructionLen + 1] = 0x25;
    memset(&buffer[instructionLen + 2], 0, 4);

    // Copy the return address to the place holder of the far jump
    memcpy(&buffer[instructionLen + 6], &retAddress, 8);

    // Write the buffer to the trampoline
    memcpy(destination, buffer.data(), length);
}

void Mem::restoreAllPatches()
{
    // Iterate through all the patches and restore them
    for (auto& patch : m_patches)
    {
        if (patch.second.hasTrampoline) removeTrampoline(reinterpret_cast<uintptr_t>(patch.second.address));
        else restore(reinterpret_cast<uintptr_t>(patch.second.address));
    }
}
