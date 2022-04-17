// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

/*
 * INMOS B008 TRAM Motherboard
 * Contains 10 TRAM slots.
 * 
 * Slot 0 is connected to the ISA bus via a C012 link adapter.
 * 
 * TODO: Emulate the C012.
 */

#include "emu.h"
#include "imsb008.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define LOG_ISA     (1U << 1)
#define LOG_LINK    (1U << 2)
#define LOG_ALL     (LOG_ISA|LOG_LINK)

#define VERBOSE     (LOG_ALL)

#include "logmacro.h"

DEFINE_DEVICE_TYPE(ISA16_IMSB008, isa16_imsb008_device, "isa_imsb008", "INMOS B008 Transputer Evaluation Board")

isa16_imsb008_device::isa16_imsb008_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, ISA16_IMSB008, tag, owner, clock)
	, device_isa16_card_interface(mconfig, *this)
	, m_sw1(*this, "SW1")
	, m_slot0(*this, "slot0")
	, m_link_adapter(*this, "link_adapter")
{
}

void isa16_imsb008_device::device_add_mconfig(machine_config &config)
{
	// C004 crossbar switch
	// C012 link adapter
	// 10 TRAM slots

	constexpr XTAL master_clock = XTAL(5'000'000);

	INMOS_SLOT(config, m_slot0, inmos_cards, "ims_b411", 1, "inmos");

	// The default configuration, which is the initial target:
	//	- C012/Link		<-> 	Slot0/Link0
	//	- Slot0/Link1 	<->		T222/Link1
	//	- T222/Link0 	<-> 	C004/Link28
	IMS_C012(config, m_link_adapter, master_clock);
}

void isa16_imsb008_device::device_config_complete()
{

}

void isa16_imsb008_device::device_resolve_objects()
{
	
}

void isa16_imsb008_device::cfg_links_ims_b411(device_t *device)
{

}

void isa16_imsb008_device::device_start()
{
	set_isa_device();

	m_trams[0] = dynamic_cast<device_inmos_tram_interface *>(m_slot0->get_card_device());
	printf("m_trams[0] = %p\n", m_trams[0]);
	
	// m_link_adapter->link_tx_cb<0>().set(*m_trams[0], FUNC(device_inmos_tram_interface::link0_in_w));	// C012 Tx -> Slot 0 Link 0 Rx
	// m_trams[0]->link_tx_cb<0>().set(m_link_adapter, FUNC(ims_c012_device::link0_rx_w));					// Slot 0 Link 0 Tx -> C012 Rx

	m_link_adapter->link_out_cb<0>().set(*m_trams[0], FUNC(device_inmos_tram_interface::inmos0_in_w));
	m_trams[0]->link_out_cb<0>().set(m_link_adapter, FUNC(device_inmos_serial_link_interface::link_in0_w));
	m_trams[0]->error_cb().set(*this, FUNC(isa16_imsb008_device::error_w));	
}

void isa16_imsb008_device::map(address_map &map)
{
	map(0x00, 0x00).rw(m_link_adapter, FUNC(ims_c012_device::input_r), FUNC(ims_c012_device::input_w));
	map(0x01, 0x01).rw(m_link_adapter, FUNC(ims_c012_device::output_r), FUNC(ims_c012_device::output_w));
	map(0x02, 0x02).rw(m_link_adapter, FUNC(ims_c012_device::input_status_r), FUNC(ims_c012_device::input_status_w));
	map(0x03, 0x03).rw(m_link_adapter, FUNC(ims_c012_device::output_status_r), FUNC(ims_c012_device::output_status_w));
	map(0x10, 0x10).rw(FUNC(isa16_imsb008_device::error_register_r), FUNC(isa16_imsb008_device::reset_register_w));
	map(0x11, 0x11).rw(FUNC(isa16_imsb008_device::analyse_register_r), FUNC(isa16_imsb008_device::analyse_register_w));
	map(0x12, 0x13).rw(FUNC(isa16_imsb008_device::dma_request_register_r), FUNC(isa16_imsb008_device::dma_request_register_w));
	map(0x13, 0x13).rw(FUNC(isa16_imsb008_device::irq_control_register_r), FUNC(isa16_imsb008_device::irq_control_register_w));
}

// The ISA RESET line does *not* reset the C012 or any TRAMs. It only resets the ISA interface.
void isa16_imsb008_device::device_reset()
{
	switch(m_sw1->read() & 0x03)
	{
		case 0: m_isa->install_device(0x0300, 0x031f, *this, &isa16_imsb008_device::map); break;
		case 1: m_isa->install_device(0x0200, 0x021f, *this, &isa16_imsb008_device::map); break;
		case 2: m_isa->install_device(0x0150, 0x016f, *this, &isa16_imsb008_device::map); break;
		case 3: break; // disabled
	}
}

// TODO: Most of these handlers
uint8_t isa16_imsb008_device::error_register_r()
{
	// returns NotError
	return (m_error > 0) ? 0 : 1;
}
void isa16_imsb008_device::error_register_w(uint8_t value)
{

	LOGMASKED(LOG_ISA, "*** TODO *** %s %d\n", FUNCNAME, value);
}

uint8_t isa16_imsb008_device::reset_register_r()
{
	if(!machine().side_effects_disabled())
	{
		LOGMASKED(LOG_ISA, "*** TODO *** %s\n", FUNCNAME);
	}
	return 0x00;
}
void isa16_imsb008_device::reset_register_w(uint8_t value)
{
	LOGMASKED(LOG_ISA, "*** TODO *** %s %d\n", FUNCNAME, value);
}

uint8_t isa16_imsb008_device::analyse_register_r()
{
	if(!machine().side_effects_disabled())
	{
		LOGMASKED(LOG_ISA, "%s\n", FUNCNAME);
	}
	return 0x00;
}
void isa16_imsb008_device::analyse_register_w(uint8_t value)
{
	LOGMASKED(LOG_ISA, "*** TODO *** %s %d\n", FUNCNAME, value);
}

uint8_t isa16_imsb008_device::dma_request_register_r()
{
	if(!machine().side_effects_disabled())
	{
		LOGMASKED(LOG_ISA, "*** TODO *** %s\n", FUNCNAME);
	}
	return 0x00;
}
void isa16_imsb008_device::dma_request_register_w(uint8_t value)
{
	LOGMASKED(LOG_ISA, "*** TODO *** %s %d\n", FUNCNAME, value);   
}

uint8_t isa16_imsb008_device::irq_control_register_r()
{
	if(!machine().side_effects_disabled())
	{
		LOGMASKED(LOG_ISA, "*** TODO *** %s\n", FUNCNAME);
	}
	return 0x00;
}
void isa16_imsb008_device::irq_control_register_w(uint8_t value)
{
	LOGMASKED(LOG_ISA, "*** TODO *** %s %d\n", FUNCNAME, value);
}

static INPUT_PORTS_START( imsb008 )
	PORT_START("SW1")
	PORT_DIPNAME(0x03, 0x00, "Base I/O Address")                        PORT_DIPLOCATION("SW1:1,2")
	PORT_DIPSETTING( 0x03, "Disabled")
	PORT_DIPSETTING( 0x02, "150h")
	PORT_DIPSETTING( 0x01, "200h")
	PORT_DIPSETTING( 0x00, "300h")
	PORT_DIPNAME(0x04, 0x00, "TRAM Slot 0 and DOWN System Services")    PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING( 0x04, "From UP")
	PORT_DIPSETTING( 0x00, "From HOST")
	PORT_DIPNAME(0x08, 0x00, "TRAM Slots 1-9 System Services")          PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING( 0x08, "From Slot 0 Subsystem")
	PORT_DIPSETTING( 0x00, "From DOWN")
 	PORT_DIPNAME(0x10, 0x10, "Links 1-3 Speed") 						PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING( 0x10, "10 Mbit/sec")
	PORT_DIPSETTING( 0x00, "20 Mbit/sec") 
 	PORT_DIPNAME(0x20, 0x20, "Link 0 Speed") 							PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING( 0x20, "10 Mbit/sec")
	PORT_DIPSETTING( 0x00, "20 Mbit/sec")
INPUT_PORTS_END

ioport_constructor isa16_imsb008_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( imsb008 );
}