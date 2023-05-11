#ifndef CPU_INMOS_TRANSPUTER_LINK
#define CPU_INMOS_TRANSPUTER_LINK

// 	Channel Control Words
//	These are special locations in memory that trigger a Tx or Rx on a link.
//
void transputer_cpu_device::link0_ccw_w(uint8_t data)
{
	m_links[0].send_byte(data, serial_get_next_edge());
}

void transputer_cpu_device::link1_ccw_w(uint8_t data)
{
	m_links[1].send_byte(data, serial_get_next_edge());
}

void transputer_cpu_device::link2_ccw_w(uint8_t data)
{
	m_links[2].send_byte(data, serial_get_next_edge());
}

void transputer_cpu_device::link3_ccw_w(uint8_t data)
{
	m_links[3].send_byte(data, serial_get_next_edge());
}

// Reading the Channel Control Words
uint8_t transputer_cpu_device::link0_ccw_r()
{
	return m_links[0].rx_buffer;
}

uint8_t transputer_cpu_device::link1_ccw_r()
{
	return m_links[1].rx_buffer;
}

uint8_t transputer_cpu_device::link2_ccw_r()
{
	return m_links[2].rx_buffer;
}

uint8_t transputer_cpu_device::link3_ccw_r()
{
	return m_links[3].rx_buffer;
}

void transputer_cpu_device::internal_update(uint64_t current_time)
{
    uint64_t event_time = 0;

    // INMOS links
	for(int i=0; i != 4; i++) {
		// Calling update changes the next event time

 		// Do we need to perform a Tx event this update?
		if(m_links[i].tx_next_event && (current_time >= m_links[i].tx_next_event))
        {
			std::string old_state = InmosLink::TxState_to_String(m_links[i].tx_state);
            serial_tx_update(i, current_time);
			std::string new_state = InmosLink::TxState_to_String(m_links[i].tx_state);

        	LOGMASKED(LOG_SERIAL, "Link%d Tx state %s -> %s - Tx=%d - Next %d - Cycle %d\n", i, old_state, new_state, m_links[i].tx_value, m_links[i].tx_next_event, current_time);
            machine().scheduler().synchronize(timer_expired_delegate(FUNC(device_inmos_serial_link_interface::perform_write_cb), this), i);
        }

		// Recalculate the next update time.
		if(m_links[i].tx_next_event && (!event_time || m_links[i].tx_next_event < event_time))
			event_time = m_links[i].tx_next_event;
	}

    recompute_bcount(event_time);
}

// Someone wrote to the CPU's link. These are BITS, not BYTES.
WRITE_LINE_MEMBER(transputer_cpu_device::link_in0_w)
{
	m_links[0].rx_value = state > 0;
	if(m_links[0].rx_state == InmosLink::RxState::RX_IDLE) m_links[0].rx_state = InmosLink::RxState::RX_STARTBIT;
    m_links[0].rx_next_event = total_cycles();

	machine().scheduler().synchronize(timer_expired_delegate(FUNC(device_inmos_serial_link_interface::perform_read_cb), this), 0);
}
WRITE_LINE_MEMBER(transputer_cpu_device::link_in1_w)
{

}
WRITE_LINE_MEMBER(transputer_cpu_device::link_in2_w)
{

}
WRITE_LINE_MEMBER(transputer_cpu_device::link_in3_w)
{

}

void transputer_cpu_device::received_full_byte_cb(InmosLink::LinkId link)
{
    LOGMASKED(LOG_SERIAL, "received a full byte %02X\n", m_links[link].rx_buffer);

	m_links[0].send_ack_packet(total_cycles());
	m_links[0].rx_byte_present = true;
}

void transputer_cpu_device::perform_read_cb(s32 link_num)
{
    // Callback from the Rx write handler.
	std::string old_state = InmosLink::RxState_to_String(m_links[link_num].rx_state);
	serial_rx_update(link_num, 0);
	std::string new_state = InmosLink::RxState_to_String(m_links[link_num].rx_state);
	LOGMASKED(LOG_SERIAL, "Link%d Rx state %s -> %s - Rx=%d\n", link_num, old_state, new_state, m_links[link_num].rx_value);
}

void transputer_cpu_device::got_ack_packet_cb()
{
    //
}

void transputer_cpu_device::link_tx_is_ready(InmosLink::LinkId link)
{
	//
}

void transputer_cpu_device::recompute_bcount(uint64_t event_time)
{
	if(!event_time || event_time >= total_cycles() + m_icount) {
		m_bcount = 0;
		return;
	}
	m_bcount = total_cycles() + m_icount - event_time;
}

u64 transputer_cpu_device::serial_get_next_edge()
{
    // 10Mbit/sec, so bits are 100ns apart.
	u64 current_time = total_cycles();
	u64 step = m_event_step;
	return current_time+step;
}

#endif
