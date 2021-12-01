// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*
 * Motorola MVME320 VMEbus Disk Controller Module
 *
 * ST-506 and Shugart SA-400 disk controller, FM and MFM.
 * Supports 8" and 5.25" disk drives plus ST-506 hard drives.
 * 
 * Consists of a Signetics 8X305 running sequencer firmware and a bunch of TTL.
 *
 * There are three versions of the MVME320:
 * - MVME320: Has weird connectors and requires an MVME901 adapter board to connect to drives.
 * - MVME320/A: Simplified layout, standard connectors.
 * - MVME320/B: Adds support for 1.2M floppy drives.
 *  */

#include "emu.h"
#include "vme_mvme320.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define LOG_PRINTF  (1U << 1)
#define LOG_SETUP   (1U << 2)
#define LOG_GENERAL (1U << 3)

#define VERBOSE (LOG_PRINTF | LOG_SETUP | LOG_GENERAL)

#include "logmacro.h"

#define LOGPRINTF(...)  LOGMASKED(LOG_PRINTF,   __VA_ARGS__)
#define LOGSETUP(...)   LOGMASKED(LOG_SETUP,    __VA_ARGS__)
#define LOGGENERAL(...) LOGMASKED(LOG_GENERAL,  __VA_ARGS__)

DEFINE_DEVICE_TYPE(MVME320, vme_mvme320_device, "mvme320", "Motorola MVME320 Disk Controller")

vme_mvme320_device::vme_mvme320_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock)
	, device_vme_card_interface(mconfig, *this)
	, m_maincpu(*this, "maincpu")
{

}

// ROM definitions
ROM_START(mvme320b)
    ROM_REGION16_LE(0x2000, "maincpu", 0)
    ROM_LOAD16_BYTE("U1_3.0.bin", 0x0000, 0x1000, CRC(87d62dac) SHA1(c57eb9f8aefe29794b8fc5f0afbaff9b59d38c73), ROM_BIOS(0))
	ROM_LOAD16_BYTE("U2_3.0.bin", 0x0001, 0x1000, CRC(5651b61d) SHA1(0d0004dff3c88b2f0b18951b4f2acd7f65f701b1), ROM_BIOS(0))

	ROM_REGION(0x1000, "prom", 0) // Addressing logic?
	ROM_LOAD("U4_3.0.bin", 0x000, 0x400, CRC(fcd31bff) SHA1(ae34c6eb6659dc992896b388be1badfab0fd7971))
ROM_END