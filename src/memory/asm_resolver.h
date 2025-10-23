#pragma once

class AsmResolver
{
public:
    explicit AsmResolver(uintptr_t addr)
        : m_module(0)
        , m_address(addr)
    {
    }

    AsmResolver(uintptr_t mod, uintptr_t addr)
        : m_module(mod)
        , m_address(addr)
    {
    }

    uintptr_t getAddress() const { return m_address; }

    uintptr_t relativeJMP() const;
    uintptr_t relativeCALL() const;
    uintptr_t relativeMOV() const;
    uintptr_t relativeLEA() const;
    uintptr_t relativeSIMD() const;

    uintptr_t JMP() const;
    uintptr_t CALL() const;
    uintptr_t MOV() const;
    uintptr_t LEA() const;
    uintptr_t SIMD() const;

private:
    uintptr_t m_module;
    uintptr_t m_address;
};
