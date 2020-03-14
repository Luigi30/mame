// license:BSD-3-Clause
// copyright-holders:LuigiThirty

#include "emu.h"

#include "imagedev/harddriv.h"

#include "cpu/z80/z80.h"
#include "machine/clock.h"

#include "bus/rc2014/rc2014.h"
#include "bus/rc2014/cf.h"
#include "bus/rc2014/dual_joystick.h"
#include "bus/rc2014/dual_serial.h"
#include "bus/rc2014/vdp_video.h"
#include "bus/rc2014/tms_video.h"

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
	{ }

	void rc2014(machine_config &config);

	void init_rc2014();

	DECLARE_WRITE_LINE_MEMBER(incoming_nmi);

private:

	DECLARE_READ8_MEMBER(mem_r);
	DECLARE_WRITE8_MEMBER(mem_w);

	virtual void machine_start() override;
	virtual void machine_reset() override;

	void io_map(address_map &map);
	void mem_map(address_map &map);
	void tms_map(address_map &map);

	required_device<cpu_device> m_maincpu;
	required_device<rc2014_bus_device> m_rc2014;

	int m_last_nmi_state;
};

WRITE_LINE_MEMBER(rc2014_state::incoming_nmi)
{
	// NMI on rising edge
	if (state && !m_last_nmi_state)
		m_maincpu->pulse_input_line(INPUT_LINE_NMI, attotime::zero);

	m_last_nmi_state = state;
}

void rc2014_state::machine_start()
{
	save_item(NAME(m_last_nmi_state));
};

void rc2014_state::machine_reset()
{
	m_last_nmi_state = 0;
}

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
	device.option_add("dual_joystick", RC2014_DUAL_JOYSTICK);

	// experimental don't commit this
	device.option_add("tms_video", RC2014_TMS_VIDEO);
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
    RC2014_SLOT(config, "rc2014:4", m_rc2014, m_maincpu, rc2014_rc2014_devices, "dual_joystick");
	RC2014_SLOT(config, "rc2014:5", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
    RC2014_SLOT(config, "rc2014:6", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
    RC2014_SLOT(config, "rc2014:7", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
    RC2014_SLOT(config, "rc2014:8", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
	RC2014_SLOT(config, "rc2014:9", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);
    RC2014_SLOT(config, "rc2014:10", m_rc2014, m_maincpu, rc2014_rc2014_devices, nullptr);

	// m_rc2014->irq_wr_callback().set_inputline(m_maincpu, INPUT_LINE_IRQ0);
	// m_rc2014->nmi_wr_callback().set_inputline(m_maincpu, INPUT_LINE_NMI);
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
