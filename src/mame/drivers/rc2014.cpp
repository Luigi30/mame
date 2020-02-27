// license:BSD-3-Clause
// copyright-holders:LuigiThirty

#include "emu.h"

#include "imagedev/harddriv.h"

#include "cpu/z80/z80.h"
#include "machine/clock.h"

#include "bus/rc2014/rc2014.h"
#include "bus/rc2014/cf.h"
#include "bus/rc2014/dual_serial.h"
#include "bus/rc2014/vdp_video.h"

#include "video/tms9928a.h"

#define SCREEN_TAG			"screen"
#define BUS_TAG				"rc2014"

#define MASTER_CLOCK 		7.3728_MHz_XTAL

class rc2014_state: public driver_device
{
public:
	rc2014_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_rc2014(*this, BUS_TAG)
		
		, m_tms34010
	{ }

	void rc2014(machine_config &config);

	void init_rc2014();

private:

	DECLARE_READ8_MEMBER(mem_r);
	DECLARE_WRITE8_MEMBER(mem_w);

	virtual void machine_start() override;

	void io_map(address_map &map);
	void mem_map(address_map &map);
	void tms_map(address_map &map);

	required_device<cpu_device> m_maincpu;
	required_device<rc2014_bus_device> m_rc2014;
};

void rc2014_state::machine_start()
{
	
};

void rc2014_state::mem_map(address_map &map)
{
	// map.unmap_value_high();
	map(0x0000, 0x1fff).rom();
	map(0x2000, 0xffff).ram();
}

void rc2014_state::io_map(address_map &map)
{
	/* I/O devices... */
	map.unmap_value_high();
	map.global_mask(0xff);
}

static void rc2014_rc2014_devices(device_slot_interface &device)
{
	device.option_add("cf", RC2014_CF);
	device.option_add("dual_serial", RC2014_DUAL_SERIAL);
	device.option_add("vdp_video", RC2014_VDP_VIDEO);
}

void rc2014_state::rc2014(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, MASTER_CLOCK);
	m_maincpu->set_addrmap(AS_PROGRAM, &rc2014_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &rc2014_state::io_map);	

	RC2014_BUS(config, m_rc2014, 0);
	RC2014_SLOT(config, "rc2014:1", m_rc2014, m_maincpu, rc2014_rc2014_devices, "cf");
    RC2014_SLOT(config, "rc2014:2", m_rc2014, m_maincpu, rc2014_rc2014_devices, "dual_serial");
    RC2014_SLOT(config, "rc2014:3", m_rc2014, m_maincpu, rc2014_rc2014_devices, "vdp_video");
    RC2014_SLOT(config, "rc2014:4", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
	RC2014_SLOT(config, "rc2014:5", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
    RC2014_SLOT(config, "rc2014:6", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
    RC2014_SLOT(config, "rc2014:7", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
    RC2014_SLOT(config, "rc2014:8", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
	RC2014_SLOT(config, "rc2014:9", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
    RC2014_SLOT(config, "rc2014:10", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
}

void rc2014_state::init_rc2014()
{

}

INPUT_PORTS_START( rc2014 )

INPUT_PORTS_END

/* ROM definition */
ROM_START( rc2014 )
	ROM_REGION( 0x2000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "rc2014.bin", 0x0000, 0x2000, CRC(dd5b9cd9) SHA1(97c176fcb63674f0592851b7858cb706886b5857))
ROM_END

/*    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT   CLASS         INIT         COMPANY        FULLNAME         FLAGS */
COMP( 2014, rc2014, 0,      0,      rc2014,  rc2014, rc2014_state, empty_init, "RFC2795 Ltd", "RC2014 Pro", MACHINE_NO_SOUND_HW)
