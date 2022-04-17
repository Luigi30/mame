// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

    INMOS C012 Link Adapter emulation

***************************************************************************/

#include "emu.h"
#include "imsc012.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define LOG_LINK    (1U << 1)
#define LOG_SERIAL  (1U < 2)
#define VERBOSE     (LOG_LINK|LOG_SERIAL)

#include "logmacro.h"

DEFINE_DEVICE_TYPE(IMS_C012, ims_c012_device, "ims_c012", "INMOS C012 Link Adapter")

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  imsc012_device - constructor
//-------------------------------------------------

ims_c012_device::ims_c012_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	  device_t(mconfig, IMS_C012, tag, owner, clock)
    , device_execute_interface(mconfig, *this)
    , device_inmos_serial_link_interface(mconfig, *this)
    , m_tx_cb{*this}
    , m_input_int_func(*this)
	, m_output_int_func(*this)
    , m_icount(0)
{
}

ims_c012_device::ims_c012_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: ims_c012_device(mconfig, IMS_C012, tag, owner, clock)
{
}

//-------------------------------------------------
//  imsc012_device - device-level overrides
//-------------------------------------------------

void ims_c012_device::device_start()
{
    m_event_step = 100;

    m_link_out_cb.resolve_all_safe();
    set_icountptr(m_icount);
}

void ims_c012_device::device_reset()
{
    m_input_data_present = false;
    m_input_interrupt_enable = false;
    m_output_ready = true;
    m_output_interrupt_enable = false;

    m_links[0].reset();
}

void ims_c012_device::device_resolve_objects()
{
    m_input_int_func.resolve_safe();
	m_output_int_func.resolve_safe();
}

//-------------------------------------------------
//  imsc012_device - I/O handlers
//-------------------------------------------------

//-------------------------------------------------
//  Input:  C012Link -> ISA
//-------------------------------------------------
uint8_t ims_c012_device::input_r()
{
    // Clears the input data present bit on read.
    // If input data present, sends an ACK packet to link.

    if(m_input_data_present) { m_links[0].send_ack_packet(total_cycles()); }

    m_input_data_present = false;
    return m_input_data;
}

void ims_c012_device::input_w(uint8_t value)
{
    // No effect on writes.
}

//-------------------------------------------------
//  Input Status Register
//  Bit 0 - A byte is present in the input buffer. Read-only, must be written as 0.
//  Bit 1 - When set, the interrupt controller will raise an IRQ each time bit 0 is set.
//  Bits 2-7 are unused and must be written as 0.
//
//  On link reset, both bits are reset to 0.
//-------------------------------------------------
uint8_t ims_c012_device::input_status_r()
{
    uint8_t data = 0;
    data |= m_input_data_present;
    data |= (m_input_interrupt_enable << 1);

    return data;
}

void ims_c012_device::input_status_w(uint8_t state)
{
    m_input_interrupt_enable = BIT(state, 1);
}


//-------------------------------------------------
//  Output: ISA -> C012Link
//-------------------------------------------------
uint8_t ims_c012_device::output_r()
{
    // Read value is undefined.
    return 0xff;
}

void ims_c012_device::output_w(uint8_t value)
{
    m_output_ready = false;
    LOGMASKED(LOG_LINK, "%s: output_w %02X (ISA -> serial)\n", FUNCNAME, value);
    m_links[0].send_byte(value, serial_get_next_edge());
}

//-------------------------------------------------
//  Output Status Register
//  Bit 0 - When set, the output port is empty. Writing to the output port clears this bit.
//  Bit 1 - When set, the interrupt controller will raise an IRQ each time bit 0 is set.
//  Bits 2-7 are unused and must be written as 0.
//
//  On link reset, bit 0 is set and bit 1 is cleared.
//-------------------------------------------------
uint8_t ims_c012_device::output_status_r()
{
    // Bit 1 - interrupt enabled
    // Bit 0 - Output port ready
    uint8_t data = 0;
    data |= m_output_ready;
    data |= (m_output_interrupt_enable << 1);
    return data;
}

void ims_c012_device::output_status_w(uint8_t state)
{
    m_output_interrupt_enable = BIT(state, 1);
}

//-------------------------------------------------
//  Execute loop for serial transfer.
//-------------------------------------------------
void ims_c012_device::execute_run()
{
    do
    {
        internal_update(total_cycles());
        m_icount--;
    } while (m_icount > 0);
};

void ims_c012_device::recompute_bcount(uint64_t event_time)
{
	if(!event_time || event_time >= total_cycles() + m_icount) {
		m_bcount = 0;
		return;
	}
	m_bcount = total_cycles() + m_icount - event_time;
}

//-------------------------------------------------
//  Write line for receiving data over C012Link.
//-------------------------------------------------
WRITE_LINE_MEMBER(ims_c012_device::link_in0_w)
{
	m_links[0].rx_value = state > 0;
	if(m_links[0].rx_state == InmosLink::RxState::RX_IDLE) m_links[0].rx_state = InmosLink::RxState::RX_STARTBIT;
    if(m_links[0].rx_next_event == 0) m_links[0].rx_next_event = total_cycles();

    machine().scheduler().synchronize(timer_expired_delegate(FUNC(device_inmos_serial_link_interface::perform_read_cb), this), 0);
}

//-------------------------------------------------
//  internal_update: Called each cycle in execute_run to handle serial I/O.
//-------------------------------------------------
void ims_c012_device::internal_update(uint64_t current_time)
{
    // C012 only has one link.

    uint64_t event_time = 0;

    // Calling update changes the next event time  

    // Do we need to perform a Tx event this update?
    if(m_links[0].tx_next_event && current_time >= m_links[0].tx_next_event)
    {
        std::string old_state = InmosLink::TxState_to_String(m_links[0].tx_state);
        serial_tx_update(0, current_time);
        std::string new_state = InmosLink::TxState_to_String(m_links[0].tx_state);

        LOGMASKED(LOG_SERIAL, "Link%d Tx state %s -> %s - Tx=%d - NeedsAck=%d - Next %d - Cycle %d\n", 0, old_state, new_state, m_links[0].tx_value, m_links[0].waiting_for_ack, m_links[0].tx_next_event, current_time);
        machine().scheduler().synchronize(timer_expired_delegate(FUNC(device_inmos_serial_link_interface::perform_write_cb), this), 0);
    }

    // Recalculate the next update time.
    if(m_links[0].tx_next_event && (!event_time || m_links[0].tx_next_event < event_time))
        event_time = m_links[0].tx_next_event;

    recompute_bcount(event_time);
}

u64 ims_c012_device::serial_get_next_edge()
{
    // 10Mbit/sec, so bits are 100ns apart.
	u64 current_time = total_cycles();
	return current_time+m_event_step;
}

//-------------------------------------------------
//  Write line for receiving data over C012Link.
//-------------------------------------------------
void ims_c012_device::received_full_byte_cb(InmosLink::LinkId link)
{
    LOGMASKED(LOG_SERIAL, "received a full byte %02X\n", m_links[0].rx_buffer);
    m_input_data = m_links[0].rx_buffer;
    m_input_data_present = true;
}

void ims_c012_device::perform_read_cb(s32 link_num)
{
    // Callback from the Rx write handler.
	std::string old_state = InmosLink::RxState_to_String(m_links[link_num].rx_state);
	serial_rx_update(link_num, 0);
	std::string new_state = InmosLink::RxState_to_String(m_links[link_num].rx_state);
	LOGMASKED(LOG_SERIAL, "Link%d Rx state %s -> %s - Rx=%d\n", 0, old_state, new_state, m_links[0].rx_value);
}

void ims_c012_device::got_ack_packet_cb()
{
    m_output_ready = true;
}