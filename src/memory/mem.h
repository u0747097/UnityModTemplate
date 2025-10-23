#pragma once

#define AUTO_ASSEMBLE_TRAMPOLINE(ADDRESS, TRAMPOLINE_LENGTH, INSTRUCTIONS) \
do { \
auto allocMemory = MemoryUtils::AllocateNearbyMemory(ADDRESS, sizeof(INSTRUCTIONS) + 14); \
MemoryUtils::CreateTrampoline(ADDRESS, allocMemory, TRAMPOLINE_LENGTH); \
MemoryUtils::WriteInstructions(allocMemory, INSTRUCTIONS, sizeof(INSTRUCTIONS), ADDRESS + TRAMPOLINE_LENGTH); \
} while (false)

class Mem
{
public:
    static void patch(void* address, const char* bytes, size_t len);
    static void patch(uintptr_t address, const char* bytes);

    template <size_t N>
    static void patch(uintptr_t address, const BYTE (&bytes)[N]);

    static void nop(uintptr_t address, size_t len);
    static void restore(uintptr_t address);
    static void restore(std::initializer_list<uintptr_t> addresses);

    static uintptr_t patternScan(uintptr_t module, const char* signature);

    static void* allocateNearbyMemory(uintptr_t address, size_t size);

    static void createTrampoline(uintptr_t address, void* destination, size_t length);
    static void removeTrampoline(uintptr_t address);
    static void writeInstructions(void* destination, const BYTE instructions[], size_t instructionLen,
                                  uintptr_t retAddress);

    static void restoreAllPatches();

    struct PatchInfo
    {
        void* address;
        std::vector<BYTE> originalBytes;
        bool hasTrampoline = false;
        void* trampolineDestination = nullptr;
    };

private:
    static std::unordered_map<void*, PatchInfo> m_patches;
};

template <size_t N>
static void patch(uintptr_t address, const BYTE (&bytes)[N])
{
    PatchBytes(reinterpret_cast<void*>(address), bytes, N);
}
