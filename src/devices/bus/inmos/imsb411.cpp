// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*
 * INMOS B411 TRAM
 * A size 1 TRAM containing a T800 transputer and 1MB of SRAM.
 */

#include "emu.h"
#include "imsb411.h"

#ifndef LOGGING
#define LOGGING

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define LOG_LINK0 (1U << 1)
#define LOG_LINK1 (1U << 2)
#define LOG_LINK2 (1U << 3)
#define LOG_LINK3 (1U << 4)
#define VERBOSE (LOG_LINK0)

#include "logmacro.h"

#endif

ims_b411_device::ims_b411_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, IMS_B411, tag, owner, clock)
	, device_inmos_tram_interface(mconfig, *this)
	, m_cpu(*this, "cpu")
{
	
}

void ims_b411_device::device_start()
{
	m_link_out_cb.resolve_all_safe();
	m_error_cb.resolve_safe();
}

void ims_b411_device::device_reset()
{
	m_cpu->reset();
}

// Something wrote to one of the TRAM's links.
void ims_b411_device::inmos0_in_w(uint8_t state)
{
	m_cpu->link_in0_w(state);
}

void ims_b411_device::inmos0_out_w(uint8_t state)
{
	// todo
	link_out_w<0>(state);
}

//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

void ims_b411_device::device_add_mconfig(machine_config &config)
{
	T800(config, m_cpu, 5_MHz_XTAL);  // 5MHz external clock multiplied to 20MHz
	m_cpu->set_addrmap(AS_PROGRAM, &ims_b411_device::cpu_map);
	m_cpu->link_out_cb<0>().set(FUNC(ims_b411_device::inmos0_out_w));
	m_cpu->error_cb().set(FUNC(ims_b411_device::error_w));
}

void ims_b411_device::cpu_map(address_map &map)
{
	map(0x80001000, 0x80100fff).ram();  // 1MB of DRAM
}

DEFINE_DEVICE_TYPE(IMS_B411, ims_b411_device, "ims_b411", "INMOS B411 Transputer Module")