// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*
 * Motorola MVME320 VMEbus Disk Controller Module
 *
 * ST-506 and Shugart SA-400 disk controller, FM and MFM.
 * Supports 8" and 5.25" disk drives plus ST-506 hard drives.
 * 
 * Consists of a Signetics N8X305 running sequencer firmware and a bunch of TTL for controlling disks.
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

DEFINE_DEVICE_TYPE(VME_MVME320, vme_mvme320_card_device, "mvme320", "Motorola MVME320/B Disk Controller")

vme_mvme320_device::vme_mvme320_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock)
	, device_vme_card_interface(mconfig, *this)
	, m_maincpu(*this, "maincpu")
{

}

vme_mvme320_card_device::vme_mvme320_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme320_card_device(mconfig, VME_MVME320, tag, owner, clock)
{

}

void vme_mvme320_device::device_start()
{
	unscramble_proms();
}

void vme_mvme320_device::device_reset()
{

}

void vme_mvme320_device::unscramble_proms()
{
	// Address lines are reversed between the CPU and PROMs.
	// Unscramble them before we start.
	int length = memregion("maincpu")->bytes() / 2; // this is in words
	u16 *rom = (u16 *)memregion("maincpu")->base();
	std::unique_ptr<u16[]> tmp = std::make_unique<u16[]>(length);

	memcpy(tmp.get(), rom, length*2); // and this is in bytes
	for (int i = 0; i < length; i++)
	{
		int addr = bitswap<12>(i,0,1,2,3,4,5,6,7,8,9,10,11);
		rom[i] = tmp[addr];

		printf("swapping ROM: %04X -> %04X\n", addr, i);
	}	
}

/*
 * Machine configuration
 */
void vme_mvme320_device::device_add_mconfig(machine_config &config)
{
	// basic machine hardware
	N8X305(config, m_maincpu, 8_MHz_XTAL); // N8X305
	m_maincpu->set_addrmap(AS_PROGRAM, &vme_mvme320_device::mvme320_program_map);
	m_maincpu->set_addrmap(AS_IO, &vme_mvme320_device::mvme320_io_map);
}

void vme_mvme320_device::mvme320_program_map(address_map &map)
{
	// the A bus, 16-bit
	map(0x0000, 0x0fff).rom().region("maincpu", 0x0000);
}

void vme_mvme320_device::mvme320_io_map(address_map &map)
{
	// the IV bus, 8-bit
	//map(0x0000, 0x03ff).ram();
}

// ROM definitions
ROM_START(mvme320)
    ROM_REGION16_LE(0x2000, "maincpu", 0)
	ROM_LOAD16_BYTE("u1-3.0.bin", 0x0000, 0x1000, CRC(5651b61d) SHA1(0d0004dff3c88b2f0b18951b4f2acd7f65f701b1))
    ROM_LOAD16_BYTE("u2-3.0.bin", 0x0001, 0x1000, CRC(87d62dac) SHA1(c57eb9f8aefe29794b8fc5f0afbaff9b59d38c73))

	ROM_REGION(0x1000, "prom", 0) // Addressing logic?
	ROM_LOAD("u4-3.0.bin", 0x0000, 0x1000, CRC(fcd31bff) SHA1(ae34c6eb6659dc992896b388be1badfab0fd7971))
ROM_END

const tiny_rom_entry *vme_mvme320_device::device_rom_region() const
{
	return ROM_NAME(mvme320);
}