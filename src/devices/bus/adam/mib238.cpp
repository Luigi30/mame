// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    Coleco Adam Multipurpose Interface Board 238
    https://github.com/epearsoe/MIB238

    - 27256 socket for boot ROM is IC4

**********************************************************************/

#include "emu.h"
#include "mib238.h"

#define VERBOSE 1
#include "logmacro.h"

#define DUART_TAG       "duart"
#define SERIAL1_TAG     "j3"
#define SERIAL2_TAG     "j4"
#define LPT_TAG         "lpt"
#define LPT_LATCH_TAG   "lpt_latch"

//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

DEFINE_DEVICE_TYPE(MIB238, mib238_device, "mib238", "Multipurpose Interface Board 238")

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  mib238_device - constructor
//-------------------------------------------------

mib238_device::mib238_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, MIB238, tag, owner, clock)
	, device_adam_expansion_slot_card_interface(mconfig, *this)
    , m_duart(*this, DUART_TAG)
    , m_serial1(*this, SERIAL1_TAG)
    , m_serial2(*this, SERIAL2_TAG)
    , m_lpt(*this, LPT_TAG)
    , m_lpt_data_out(*this, LPT_LATCH_TAG)
{
}

//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

void mib238_device::device_add_mconfig(machine_config &config)
{
	SCN2681(config, m_duart, 3.6864_MHz_XTAL);
	m_duart->a_tx_cb().set(m_serial1, FUNC(rs232_port_device::write_txd));
	m_duart->b_tx_cb().set(m_serial2, FUNC(rs232_port_device::write_txd));
    m_duart->outport_cb().set(FUNC(mib238_device::duart_out_w));
    m_duart->irq_cb().set(DEVICE_SELF_OWNER, FUNC(adam_expansion_slot_device::irq_w));

	RS232_PORT(config, m_serial1, default_rs232_devices, nullptr);
	m_serial1->rxd_handler().set(m_duart, FUNC(scn2681_device::rx_a_w));
    m_serial1->cts_handler().set(m_duart, FUNC(scn2681_device::ip0_w));

	RS232_PORT(config, m_serial2, default_rs232_devices, nullptr);
	m_serial2->rxd_handler().set(m_duart, FUNC(scn2681_device::rx_b_w));
    m_serial2->cts_handler().set(m_duart, FUNC(scn2681_device::ip1_w));

    CENTRONICS(config, m_lpt, centronics_devices, "printer");

    OUTPUT_LATCH(config, m_lpt_data_out);
	m_lpt->set_output_latch(*m_lpt_data_out);
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void mib238_device::device_start()
{
}

//-------------------------------------------------
//  adam_bd_r - buffered data read
//-------------------------------------------------

uint8_t mib238_device::adam_bd_r(offs_t offset, uint8_t data, int bmreq, int biorq, int aux_rom_cs, int cas1, int cas2)
{
    // Reading I/O ports.
	if (!biorq)
	{
        if(offset >= 0x10 && offset <= 0x1F)
        {
            return m_duart->read(offset & 0x0F);
        }
        else if(offset == 0x40)
        {
            // Read printer status. b1 = busy
        }
	}

	return data;
}


//-------------------------------------------------
//  adam_bd_w - buffered data write
//-------------------------------------------------

void mib238_device::adam_bd_w(offs_t offset, uint8_t data, int bmreq, int biorq, int aux_rom_cs, int cas1, int cas2)
{
    // Writing I/O ports.
	if (!biorq)
	{
        switch (offset)
        {
            case 0x01:
                if (BIT(data, 3)) device_reset();   // Asserting bit 3 of 0x01 resets the entire card.
                break;
            case 0x1c:
                // TODO: Check this against the schematic, what does this byte actually do?
                break;
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x19:
            case 0x1a:
            case 0x1b:
            case 0x1d:
            case 0x1e:
            case 0x1f:
                return m_duart->write(offset & 0x0F, data);
                break;
            case 0x40:
                m_lpt_data_out->write(data);
                break;
        }
	}
}

void mib238_device::duart_out_w(uint8_t value)
{

}

//-------------------------------------------------
//  ROM( adam_mib238 )
//-------------------------------------------------

ROM_START( adam_mib238 )
	ROM_REGION( 0x1000, "rom", 0 )
	ROM_LOAD( "exp.rom", 0x0000, 0x1000, NO_DUMP )
ROM_END

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const tiny_rom_entry *mib238_device::device_rom_region() const
{
	return ROM_NAME( adam_mib238 );
}