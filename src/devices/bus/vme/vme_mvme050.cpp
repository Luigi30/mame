// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/*
 *	Motorola MVME050 System Controller
 *
 */

#include "emu.h"
#include "vme_mvme050.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define LOG_PRINTF  (1U << 1)
#define LOG_SETUP 	(1U << 2)
#define LOG_GENERAL (1U << 3)

#define VERBOSE (LOG_PRINTF | LOG_SETUP | LOG_GENERAL)
 
#include "logmacro.h"

#define LOGPRINTF(...) 	LOGMASKED(LOG_PRINTF, 	__VA_ARGS__)
#define LOGSETUP(...) 	LOGMASKED(LOG_SETUP, 	__VA_ARGS__)
#define LOGGENERAL(...) LOGMASKED(LOG_GENERAL, 	__VA_ARGS__)

#define MASTER_XTAL	( 32_MHz_XTAL )

DEFINE_DEVICE_TYPE(VME_MVME050,   vme_mvme050_card_device,   "mvme050",   "Motorola MVME-050")

static INPUT_PORTS_START(mvme050)

INPUT_PORTS_END

ioport_constructor vme_mvme050_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(mvme050);
}

vme_mvme050_device::vme_mvme050_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock)
	, device_vme_card_interface(mconfig, *this)
	, m_rtc (*this, "rtc")
	, m_mpcc1 (*this, "mpcc1")
	, m_mpcc2 (*this, "mpcc2")
	, m_bim1 (*this, "bim1")
	, m_bim2 (*this, "bim2")
{

}

vme_mvme050_card_device::vme_mvme050_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme050_card_device(mconfig, VME_MVME050, tag, owner, clock)
{

}

void vme_mvme050_device::mvme050_mem(address_map &map)
{
	map(0xff0000, 0xff003f).umask16(0x00ff).rw(m_mpcc1, FUNC(mpcc68561_device::read), FUNC(mpcc68561_device::write));
	map(0xff0040, 0xff007f).umask16(0x00ff).rw(m_mpcc2, FUNC(mpcc68561_device::read), FUNC(mpcc68561_device::write));
	map(0xff0100, 0xff017f).umask16(0x00ff).rw(m_rtc, FUNC(mc146818_device::read_direct), FUNC(mc146818_device::write_direct));
	map(0xff00c0, 0xff00df).umask16(0x00ff).rw(m_bim1, FUNC(bim68153_device::read), FUNC(bim68153_device::write));
	map(0xff00e0, 0xff00ff).umask16(0x00ff).rw(m_bim2, FUNC(bim68153_device::read), FUNC(bim68153_device::write));
}

void vme_mvme050_device::device_start()
{
	LOG("%s\n", FUNCNAME);
	
	m_vme->install_device(vme_device::A24_SC, 0x000000, 0xFFFFFF,
		read16_delegate(*this, FUNC(vme_mvme050_device::unmapped_vme_r)), 
		write16_delegate(*this, FUNC(vme_mvme050_device::unmapped_vme_w)),
		0xFFFFFFFF);

	m_vme->install_device(vme_device::A16_SC, 0xFF0100, 0xFF017F,
		read8sm_delegate(*subdevice<mc146818_device>("rtc"), FUNC(mc146818_device::read_direct)), 
		write8sm_delegate(*subdevice<mc146818_device>("rtc"), FUNC(mc146818_device::write_direct)), 
		0x00FF00FF);
}

void vme_mvme050_device::device_reset()
{
	LOG("%s\n", FUNCNAME);
}

static const input_device_default terminal_defaults[] =
{
	DEVICE_INPUT_DEFAULTS( "RS232_RXBAUD", 0xff, RS232_BAUD_9600 )
	DEVICE_INPUT_DEFAULTS( "RS232_TXBAUD", 0xff, RS232_BAUD_9600 )
	DEVICE_INPUT_DEFAULTS( "RS232_DATABITS", 0xff, RS232_DATABITS_8 )
	DEVICE_INPUT_DEFAULTS( "RS232_PARITY", 0xff, RS232_PARITY_NONE )
	DEVICE_INPUT_DEFAULTS( "RS232_STOPBITS", 0xff, RS232_STOPBITS_1 )
	{ nullptr, 0, 0 }
};

/*
 * Machine configuration
 */
#define RS232P1_TAG      "rs232p1"
#define RS232P2_TAG      "rs232p2"
void vme_mvme050_device::device_add_mconfig(machine_config &config)
{
	MPCC68561(config, m_mpcc1, MASTER_XTAL / 4, 0, 0);
	m_mpcc1->out_txd_cb().set(RS232P1_TAG, FUNC(rs232_port_device::write_txd));
	m_mpcc1->out_dtr_cb().set(RS232P1_TAG, FUNC(rs232_port_device::write_dtr));
	m_mpcc1->out_rts_cb().set(RS232P1_TAG, FUNC(rs232_port_device::write_rts));
	m_mpcc1->out_int_cb().set(m_bim1, FUNC(bim68153_device::int1_w));
	
	MPCC68561(config, m_mpcc2, MASTER_XTAL / 4, 0, 0);
	m_mpcc2->out_txd_cb().set(RS232P2_TAG, FUNC(rs232_port_device::write_txd));
	m_mpcc2->out_dtr_cb().set(RS232P2_TAG, FUNC(rs232_port_device::write_dtr));
	m_mpcc2->out_rts_cb().set(RS232P2_TAG, FUNC(rs232_port_device::write_rts));
	m_mpcc2->out_int_cb().set(m_bim2, FUNC(bim68153_device::int3_w));
	
	rs232_port_device &rs232p1(RS232_PORT(config, RS232P1_TAG, default_rs232_devices, nullptr));
	rs232p1.rxd_handler().set(m_mpcc1, FUNC(mpcc68561_device::write_rx));
	rs232p1.cts_handler().set(m_mpcc1, FUNC(mpcc68561_device::cts_w));

	// MPCC2 - RS232
	rs232_port_device &rs232p2(RS232_PORT(config, RS232P2_TAG, default_rs232_devices, nullptr));
	rs232p2.rxd_handler().set(m_mpcc2, FUNC(mpcc68561_device::write_rx));
	rs232p2.cts_handler().set(m_mpcc2, FUNC(mpcc68561_device::cts_w));
	
	MC68153(config, m_bim1, MASTER_XTAL / 8);
	//m_bim1->out_int_callback().set(FUNC(vme_mvme050_device::bim1_irq_callback));

	MC68153(config, m_bim2, MASTER_XTAL / 8);
	//m_bim2->out_int_callback().set(FUNC(vme_mvme050_device::bim2_irq_callback));

	MC146818(config, m_rtc, 4.194304_MHz_XTAL);
	
	VME(config, "vme", 0);
}

// The MVME050, acting as SYSCON, will assert the VME bus error line on unmapped access after
// a user-configurable timeout. TODO: the timing delay
uint16_t vme_mvme050_device::unmapped_vme_r(address_space &space, offs_t address, uint16_t mem_mask)
{
	LOG("VME bus error!\n");
	
	m_vme->berr_w(ASSERT_LINE);
	m_vme->berr_w(CLEAR_LINE);
	return 0xFFFF;
}

void vme_mvme050_device::unmapped_vme_w(address_space &space, offs_t address, uint16_t data, uint16_t mem_mask)
{
	LOG("VME bus error!\n");
	
	m_vme->berr_w(ASSERT_LINE);
	m_vme->berr_w(CLEAR_LINE);
}

// ROM definitions
ROM_START (mvme050)
ROM_END

const tiny_rom_entry *vme_mvme050_device::device_rom_region() const
{
	return ROM_NAME(mvme050);
}
