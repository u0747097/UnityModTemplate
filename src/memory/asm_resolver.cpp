#include "pch.h"
#include "asm_resolver.h"

uintptr_t AsmResolver::relativeJMP() const
{
    auto code = reinterpret_cast<BYTE*>(m_address);
    if (code[0] != 0xE9) return m_address;
    return m_address + *reinterpret_cast<int32_t*>(&code[1]) + 5;
}

uintptr_t AsmResolver::relativeCALL() const
{
    auto code = reinterpret_cast<BYTE*>(m_address);
    if (code[0] != 0xE8) return m_address;
    return m_address + *reinterpret_cast<int32_t*>(&code[1]) + 5;
}

uintptr_t AsmResolver::relativeMOV() const
{
    auto code = reinterpret_cast<BYTE*>(m_address);
    // for (int i = 0; i < 7; i++)
    // {
    // 	LOG_DEBUG("Byte[{}]: 0x{:02X}", i, code[i]);
    // }

    int offset = 0;

    bool hasREX = code[0] >= 0x40 && code[0] <= 0x4F;
    if (hasREX) offset++;

    // Check MOV load
    if (code[offset] != 0x8B && code[offset] != 0x89) return m_address;
    offset++;

    // Check ModR/M for RIP relative addressing
    BYTE modrm = code[offset];
    if ((modrm & 0xC7) != 0x05) return m_address;
    offset++;

    int32_t displacement = *reinterpret_cast<int32_t*>(&code[offset]);
    return m_address + displacement + offset + 4;
}

uintptr_t AsmResolver::relativeLEA() const
{
    auto code = reinterpret_cast<BYTE*>(m_address);
    int offset = 0;

    bool hasREX = code[0] >= 0x40 && code[0] <= 0x4F;
    if (hasREX) offset++;

    // Check LEA opcode
    if (code[offset] != 0x8D) return m_address;
    offset++;

    // Check ModR/M for RIP relative addressing
    BYTE modrm = code[offset];
    if ((modrm & 0xC7) != 0x05) return m_address;
    offset++;

    int32_t displacement = *reinterpret_cast<int32_t*>(&code[offset]);
    return m_address + displacement + offset + 4;
}

uintptr_t AsmResolver::relativeSIMD() const
{
    auto code = reinterpret_cast<BYTE*>(m_address);
    int offset = 0;

    // Skip legacy prefixes
    while (offset < 4 && (code[offset] == 0x66 || code[offset] == 0xF2 || code[offset] == 0xF3))
    {
        offset++;
    }

    // Check for optional REX prefix
    if (code[offset] >= 0x40 && code[offset] <= 0x4F)
    {
        offset++;
    }

    // Must have 0x0F prefix for SSE/AVX instructions
    if (code[offset] != 0x0F) return m_address;
    offset++;

    // Check for common SSE/AVX opcodes with memory operands
    BYTE opcode = code[offset];
    bool validOpcode = (
        opcode == 0x10 || opcode == 0x11 || // MOVUPS/MOVUPD
        opcode == 0x28 || opcode == 0x29 || // MOVAPS/MOVAPD
        opcode == 0x6F || opcode == 0x7F || // MOVDQU/MOVDQA
        opcode == 0x2E || opcode == 0x2F || // UCOMISS/COMISS
        opcode == 0x51 || opcode == 0x52 || // SQRTPS/RSQRTPS
        opcode == 0x58 || opcode == 0x59 || // ADDPS/MULPS
        opcode == 0x5C || opcode == 0x5D || // SUBPS/MINPS
        opcode == 0x5E || opcode == 0x5F // DIVPS/MAXPS
    );

    if (!validOpcode) return m_address;
    offset++;

    // Check ModR/M for RIP relative addressing (mod=00, r/m=101)
    BYTE modrm = code[offset];
    if ((modrm & 0xC7) != 0x05) return m_address;
    offset++;

    int32_t displacement = *reinterpret_cast<int32_t*>(&code[offset]);
    return m_address + displacement + offset + 4;
}

uintptr_t AsmResolver::JMP() const
{
    uintptr_t addr = relativeJMP();
    if (m_module == 0) return addr;
    if (addr >= m_module)
    {
        uintptr_t rva = addr - m_module;
        return m_module + rva;
    }
    return addr;
}

uintptr_t AsmResolver::CALL() const
{
    uintptr_t addr = relativeCALL();
    if (m_module == 0) return addr;
    if (addr >= m_module)
    {
        uintptr_t rva = addr - m_module;
        return m_module + rva;
    }
    return addr;
}

uintptr_t AsmResolver::MOV() const
{
    uintptr_t addr = relativeMOV();
    if (m_module == 0) return addr;
    if (addr >= m_module)
    {
        uintptr_t rva = addr - m_module;
        return m_module + rva;
    }
    return addr;
}

uintptr_t AsmResolver::LEA() const
{
    uintptr_t addr = relativeLEA();
    if (m_module == 0) return addr;
    if (addr >= m_module)
    {
        uintptr_t rva = addr - m_module;
        return m_module + rva;
    }
    return addr;
}

uintptr_t AsmResolver::SIMD() const
{
    uintptr_t addr = relativeSIMD();
    if (m_module == 0) return addr;
    if (addr >= m_module)
    {
        uintptr_t rva = addr - m_module;
        return m_module + rva;
    }
    return addr;
}
