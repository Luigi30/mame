
// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

    t8.cpp
    Core for the INMOS transputer architecture.

    TODO:
    - everything
***************************************************************************/

#include "emu.h"
#include "transputer_dasm.h"
#include "t8.h"

DEFINE_DEVICE_TYPE(T800, t800_cpu_device, "t800", "INMOS T800")

t800_cpu_device::t800_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: transputer_cpu_device(mconfig, T800, tag, owner, clock, CPUTYPE_T800, 32)
{
}

// uint8_t t800_cpu_device::sram_read8(offs_t addr)
// { 
//     uint8_t addr_byte = 3 - (addr % 4);
//     addr = addr & 0xFFC;

//     return reinterpret_cast<u8 *>(m_internal_sram.target())[addr + addr_byte];
// }

// void t800_cpu_device::sram_write8(offs_t addr, uint8_t data)
// {
//     // turn a byte address into a word address
//     uint8_t addr_byte = 3 - (addr % 4);
//     addr = addr & 0xFFC;

//     reinterpret_cast<u8 *>(m_internal_sram.target())[addr + addr_byte] = (data);
// }