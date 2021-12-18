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
	, m_lpt (*this, "lpt")
	, m_lpt_latch (*this, "lpt_latch")
{

}

vme_mvme050_card_device::vme_mvme050_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme050_card_device(mconfig, VME_MVME050, tag, owner, clock)
{

}

void vme_mvme050_device::mvme050_mem(address_map &map)
{
	// map(0xff0000, 0xff003f).umask16(0x00ff).rw(m_mpcc1, FUNC(mpcc68561_device::read), FUNC(mpcc68561_device::write));
	// map(0xff0040, 0xff007f).umask16(0x00ff).rw(m_mpcc2, FUNC(mpcc68561_device::read), FUNC(mpcc68561_device::write));
	// map(0xff00a0, 0xff00bf).umask16(0x00ff).rw(FUNC(vme_mvme050_device::frontpanel_r), FUNC(vme_mvme050_device::frontpanel_w));
	// map(0xff00c0, 0xff00df).umask16(0x00ff).rw(m_bim1, FUNC(bim68153_device::read), FUNC(bim68153_device::write));
	// map(0xff00e0, 0xff00ff).umask16(0x00ff).rw(m_bim2, FUNC(bim68153_device::read), FUNC(bim68153_device::write));
	// map(0xff0100, 0xff017f).umask16(0x00ff).rw(m_rtc, FUNC(mc146818_device::read_direct), FUNC(mc146818_device::write_direct));
}



void vme_mvme050_device::device_start()
{
	LOG("%s\n", FUNCNAME);
	
	m_vme->install_device(vme_device::A24_SC, 0x000000, 0xFFFFFF,
		read16_delegate(*this, FUNC(vme_mvme050_device::unmapped_vme_r)), 
		write16_delegate(*this, FUNC(vme_mvme050_device::unmapped_vme_w)),
		0xFFFFFFFF);

	m_vme->install_device(vme_device::A16_SC, 0xFF1000, 0xFF103F,
		read8sm_delegate(*subdevice<mpcc68561_device>("mpcc1"), FUNC(mpcc68561_device::read)), 
		write8sm_delegate(*subdevice<mpcc68561_device>("mpcc1"), FUNC(mpcc68561_device::write)), 
		0x00FF00FF);

	m_vme->install_device(vme_device::A16_SC, 0xFF1040, 0xFF107F,
		read8sm_delegate(*subdevice<mpcc68561_device>("mpcc2"), FUNC(mpcc68561_device::read)), 
		write8sm_delegate(*subdevice<mpcc68561_device>("mpcc2"), FUNC(mpcc68561_device::write)), 
		0x00FF00FF);

	m_vme->install_device(vme_device::A16_SC, 0xFF1080, 0xFF109F,
		read8_delegate(*this, FUNC(vme_mvme050_device::printer_r)), 
		write8_delegate(*this, FUNC(vme_mvme050_device::printer_w)), 
		0x00FF00FF);

	m_vme->install_device(vme_device::A16_SC, 0xFF10A0, 0xFF10BF,
		read8_delegate(*this, FUNC(vme_mvme050_device::frontpanel_r)), 
		write8_delegate(*this, FUNC(vme_mvme050_device::frontpanel_w)), 
		0x00FF00FF);

	m_vme->install_device(vme_device::A16_SC, 0xFF10C0, 0xFF10DF,
		read8sm_delegate(*subdevice<bim68153_device>("bim1"), FUNC(bim68153_device::read)), 
		write8sm_delegate(*subdevice<bim68153_device>("bim1"), FUNC(bim68153_device::write)), 
		0x00FF00FF);

	m_vme->install_device(vme_device::A16_SC, 0xFF10E0, 0xFF10FF,
		read8sm_delegate(*subdevice<bim68153_device>("bim1"), FUNC(bim68153_device::read)), 
		write8sm_delegate(*subdevice<bim68153_device>("bim1"), FUNC(bim68153_device::write)), 
		0x00FF00FF);

	m_vme->install_device(vme_device::A16_SC, 0xFF1100, 0xFF117F,
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
	//config.set_default_layout(layout_mvme050);

	MPCC68561(config, m_mpcc1, MASTER_XTAL / 4, 0, 0);	// 8MHz crystal input
	m_mpcc1->configure_clocks(8000000, 8000000);
	m_mpcc1->out_txd_cb().set(RS232P1_TAG, FUNC(rs232_port_device::write_txd));
	m_mpcc1->out_dtr_cb().set(RS232P1_TAG, FUNC(rs232_port_device::write_dtr));
	m_mpcc1->out_rts_cb().set(RS232P1_TAG, FUNC(rs232_port_device::write_rts));
	m_mpcc1->out_int_cb().set(m_bim1, FUNC(bim68153_device::int0_w));
	
	MPCC68561(config, m_mpcc2, MASTER_XTAL / 4, 0, 0);	// 8MHz crystal input
	m_mpcc2->out_txd_cb().set(RS232P2_TAG, FUNC(rs232_port_device::write_txd));
	m_mpcc2->out_dtr_cb().set(RS232P2_TAG, FUNC(rs232_port_device::write_dtr));
	m_mpcc2->out_rts_cb().set(RS232P2_TAG, FUNC(rs232_port_device::write_rts));
	m_mpcc2->out_int_cb().set(m_bim2, FUNC(bim68153_device::int1_w));
	
	rs232_port_device &rs232p1(RS232_PORT(config, RS232P1_TAG, default_rs232_devices, nullptr));
	rs232p1.rxd_handler().set(m_mpcc1, FUNC(mpcc68561_device::write_rx));
	rs232p1.cts_handler().set(m_mpcc1, FUNC(mpcc68561_device::cts_w));

	// MPCC2 - RS232
	rs232_port_device &rs232p2(RS232_PORT(config, RS232P2_TAG, default_rs232_devices, nullptr));
	rs232p2.rxd_handler().set(m_mpcc2, FUNC(mpcc68561_device::write_rx));
	rs232p2.cts_handler().set(m_mpcc2, FUNC(mpcc68561_device::cts_w));
	
	MC68153(config, m_bim1, MASTER_XTAL / 8);
	m_bim1->out_int_callback().set(FUNC(vme_mvme050_device::bim1_irq_callback));

	MC68153(config, m_bim2, MASTER_XTAL / 8);
	m_bim2->out_int_callback().set(FUNC(vme_mvme050_device::bim2_irq_callback));

	MC146818(config, m_rtc, 4.194304_MHz_XTAL);

	CENTRONICS(config, m_lpt, centronics_devices, "printer");
	m_lpt->ack_handler().set(FUNC(vme_mvme050_device::printer_ack_w));
	m_lpt->perror_handler().set(FUNC(vme_mvme050_device::printer_perror_w));
	m_lpt->select_handler().set(FUNC(vme_mvme050_device::printer_select_w));
	m_lpt->fault_handler().set(FUNC(vme_mvme050_device::printer_fault_w));

	OUTPUT_LATCH(config, m_lpt_latch);
	m_lpt->set_output_latch(*m_lpt_latch);
	
	VME(config, "vme", 0);
}

/************************
 * Parallel port
 ************************/
// Input lines
WRITE_LINE_MEMBER(vme_mvme050_device::printer_ack_w)
{
	m_printer_ack_in = state;
}


WRITE_LINE_MEMBER(vme_mvme050_device::printer_perror_w)
{
	m_printer_perror_in = state;
}


WRITE_LINE_MEMBER(vme_mvme050_device::printer_select_w)
{
	m_printer_select_in = state;
}


WRITE_LINE_MEMBER(vme_mvme050_device::printer_fault_w)
{
	m_printer_fault_in = state;
}

uint8_t vme_mvme050_device::printer_r(address_space &space, offs_t offset, uint8_t mem_mask)
{
	uint8_t retval = 0xFF;

	switch(offset)
	{
		case 0:
			// Reading the latch address writes $FF to the latch.
			if(machine().side_effects_disabled()) m_lpt_latch->write(0xFF);
			break;
		case 1:
			// clear printer acknowledge flag and printer IRQ
			if(machine().side_effects_disabled()) m_lpt->write_ack(0);
			break;
		case 2:
			retval = (m_printer_select_in) | (m_printer_perror_in << 1) | (m_printer_fault_in << 2) | (m_printer_ack_in << 7);
			break;
		case 3:
			break; // NOP
		case 4:
			// TODO: return /SYSFAIL
			break;
		case 5:
			break; // nop
	}

	return retval;
}

void vme_mvme050_device::printer_w(address_space &space, offs_t offset, uint8_t data, uint8_t mem_mask)
{
	switch(offset)
	{
		case 0:
			m_lpt_latch->write(data);
			break;
		case 1:
			m_lpt->write_strobe(BIT(data, 3));
			break;	// bit 3: printer strobe register
		case 2:
			break;	// nop
		case 3:
			m_lpt->write_init(BIT(data, 3));
			break;
		case 4:
			break;	// nop
		case 5:
			set_frontpanel_blanking(BIT(data, 3));
			break;
	}
}

/************************
 * BIMs
 ************************/
WRITE_LINE_MEMBER(vme_mvme050_device::bim1_irq_callback)
{
	LOG("%s(%02x)\n", FUNCNAME, state);

	bim1_irq_state = state;
	bim1_irq_level = m_bim1->get_irq_level();
	LOG(" - BIM1 irq level %s\n", bim1_irq_level == CLEAR_LINE ? "Cleared" : "Asserted");
	update_irq_to_maincpu();
}

WRITE_LINE_MEMBER(vme_mvme050_device::bim2_irq_callback)
{
	LOG("%s(%02x)\n", FUNCNAME, state);

	bim2_irq_state = state;
	bim2_irq_level = m_bim2->get_irq_level();
	LOG(" - BIM2 irq level %s\n", bim2_irq_level == CLEAR_LINE ? "Cleared" : "Asserted");
	update_irq_to_maincpu();
}

void vme_mvme050_device::update_irq_to_maincpu()
{
	LOG("%s()\n", FUNCNAME);

	// The two BIMs are both connected to the VME IRQ lines. Only one is selected at a time.
	switch (bim1_irq_level & 0x07)
	{
		case 0: break; // IRQ disabled
		case 1: vme_irq1_w(bim1_irq_state); break;
		case 2: vme_irq2_w(bim1_irq_state); break;
		case 3: vme_irq3_w(bim1_irq_state); break;
		case 4: vme_irq4_w(bim1_irq_state); break;
		case 5: vme_irq5_w(bim1_irq_state); break;
		case 6: vme_irq6_w(bim1_irq_state); break;
		case 7: vme_irq7_w(bim1_irq_state); break;
		default: logerror("Programmatic error in %s, please report\n", FUNCNAME);
	}

	switch (bim2_irq_level & 0x07)
	{
		case 0: break; // IRQ disabled
		case 1: vme_irq1_w(bim2_irq_state); break;
		case 2: vme_irq2_w(bim2_irq_state); break;
		case 3: vme_irq3_w(bim2_irq_state); break;
		case 4: vme_irq4_w(bim2_irq_state); break;
		case 5: vme_irq5_w(bim2_irq_state); break;
		case 6: vme_irq6_w(bim2_irq_state); break;
		case 7: vme_irq7_w(bim2_irq_state); break;
		default: logerror("Programmatic error in %s, please report\n", FUNCNAME);
	}
}

/************************
 * VMEbus control behavior
 ************************/
// The MVME050, acting as SYSCON, will assert the VME bus error line on unmapped access after
// a user-configurable timeout.
uint16_t vme_mvme050_device::unmapped_vme_r(address_space &space, offs_t address, uint16_t mem_mask)
{
	if(!machine().side_effects_disabled())
	{
		LOG("VME bus error: failed read $%08X\n", address*2);
		
		m_vme->berr_w(ASSERT_LINE);
		m_vme->berr_w(CLEAR_LINE);
	}
	return 0xFFFF;
}

void vme_mvme050_device::unmapped_vme_w(address_space &space, offs_t address, uint16_t data, uint16_t mem_mask)
{
	if(!machine().side_effects_disabled())
	{
		LOG("VME bus error: failed write $%08X -> $%04X\n", address*2, data);
	
		m_vme->berr_w(ASSERT_LINE);
		m_vme->berr_w(CLEAR_LINE);
	}
}

/************************
 * Front panel controls
 ************************/
void vme_mvme050_device::set_frontpanel_blanking(bool state)
{
	m_frontpanel_blanked = !state;
	// When layout is hooked up, enable or disable the front panel LEDs.
}

uint8_t	vme_mvme050_device::frontpanel_r(address_space &space, offs_t offset, uint8_t mem_mask)
{
	// TODO: Returns the S1 switch block value.
	return 0xFF;
}

void vme_mvme050_device::frontpanel_w(address_space &space, offs_t offset, uint8_t data, uint8_t mem_mask)
{
	// Latches the byte value to the output displays.
	m_frontpanel_value = data;

	LOG("Front panel LEDs now: $%02X\n", data);
}

void vme_mvme050_device::vme_sysfail_w(int state)
{
	LOG("MVME050 FAIL light now %d\n", !state); // active low
}

// ROM definitions
ROM_START (mvme050)
ROM_END

const tiny_rom_entry *vme_mvme050_device::device_rom_region() const
{
	return ROM_NAME(mvme050);
}
