
// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

    t2.cpp
    Core for the INMOS transputer architecture.

    TODO:
    - everything
***************************************************************************/

#include "emu.h"
#include "transputer_dasm.h"
#include "t2.h"

DEFINE_DEVICE_TYPE(T212, t212_cpu_device, "t212", "INMOS T212")

t212_cpu_device::t212_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: transputer_cpu_device(mconfig, T212, tag, owner, clock, CPUTYPE_T212, 16)
{
}

// uint8_t t212_cpu_device::sram_read8(offs_t addr)
// { 
//     uint8_t addr_byte = 1 - (addr % 2);
//     addr = addr & 0xFFC;

//     return reinterpret_cast<u8 *>(m_internal_sram.target())[addr + addr_byte];
// }

// void t212_cpu_device::sram_write8(offs_t addr, uint8_t data)
// {
//     // turn a byte address into a word address
//     uint8_t addr_byte = 1 - (addr % 2);
//     addr = addr & 0xFFC;

//     reinterpret_cast<u8 *>(m_internal_sram.target())[addr + addr_byte] = (data);
// }