
// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

    t4.cpp
    Core for the INMOS transputer architecture.

    TODO:
    - everything
***************************************************************************/

#include "emu.h"
#include "transputer_dasm.h"
#include "t4.h"

DEFINE_DEVICE_TYPE(T414, t414_cpu_device, "t414", "INMOS T414")

t414_cpu_device::t414_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: transputer_cpu_device(mconfig, T414, tag, owner, clock, CPUTYPE_T414, 32)
{
}

// uint8_t t414_cpu_device::sram_read8(offs_t addr)
// { 
//     uint8_t addr_byte = 3 - (addr % 4);
//     addr = addr & 0xFFC;

//     return reinterpret_cast<u8 *>(m_internal_sram.target())[addr + addr_byte];
// }
// void t414_cpu_device::sram_write8(offs_t addr, uint8_t data)
// {
//     // turn a byte address into a word address
//     uint8_t addr_byte = 3 - (addr % 4);
//     addr = addr & 0xFFC;

//     reinterpret_cast<u8 *>(m_internal_sram.target())[addr + addr_byte] = (data);
// }