// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

#include "emu.h"
#include "link.h"

std::string InmosLink::RxState_to_String(RxState rxstate)
{
	switch(rxstate)
	{
		case RX_IDLE: 					return "IDLE";
		case RX_STARTBIT:				return "STARTBIT";
		case RX_PACKETID:				return "PACKETID";
		case RX_DATABIT0:				return "DATABIT0";
		case RX_DATABIT1:				return "DATABIT1";
		case RX_DATABIT2:				return "DATABIT2";
		case RX_DATABIT3:				return "DATABIT3";
		case RX_DATABIT4:				return "DATABIT4";
		case RX_DATABIT5:				return "DATABIT5";
		case RX_DATABIT6:				return "DATABIT6";
		case RX_DATABIT7:				return "DATABIT7";
		case RX_STOPBIT:				return "STOPBIT";
		case RX_ACK_STARTBIT:			return "ACK_STARTBIT";
		case RX_ACK_ACKBIT:				return "ACK_ACKBIT";
		default: return "(error)";
	}
}

std::string InmosLink::TxState_to_String(TxState txstate)
{
	switch(txstate)
	{
		case TX_IDLE: 					return "IDLE";
		case TX_XMIT:					return "XMIT";
		case TX_STARTBIT:				return "STARTBIT";
		case TX_PACKETID:				return "PACKETID";
		case TX_DATABIT0:				return "DATABIT0";
		case TX_DATABIT1:				return "DATABIT1";
		case TX_DATABIT2:				return "DATABIT2";
		case TX_DATABIT3:				return "DATABIT3";
		case TX_DATABIT4:				return "DATABIT4";
		case TX_DATABIT5:				return "DATABIT5";
		case TX_DATABIT6:				return "DATABIT6";
		case TX_DATABIT7:				return "DATABIT7";
		case TX_STOPBIT:				return "STOPBIT";
		case TX_WAITING_FOR_ACK:		return "WAITING_FOR_ACK";

		case TX_SENDING_ACK_STARTBIT:	return "SENDING_ACK_STARTBIT";
		case TX_SENDING_ACK_ACKBIT:		return "SENDING_ACK_ACKBIT";
		default: return "(error)";
	}
}

void device_inmos_serial_link_interface::serial_rx_update(u8 link_num, u64 current_time)
{
	// Rx at the midpoint of the Tx event duration.
	u64	next = m_links[link_num].rx_next_event + m_event_step;
	u8 nstate = m_links[link_num].rx_state + 1;

	switch(m_links[link_num].rx_state)
	{
		case InmosLink::RxState::RX_IDLE:
		{
			// The receiver is idle.
			next = 0; // idle
			nstate = InmosLink::RxState::RX_IDLE;
			break;
		}
		case InmosLink::RxState::RX_STARTBIT:
		{
			// The receiver has received a start bit.

			// If start bit is not 1, we lost synchronization.
			if(m_links[link_num].rx_value == CLEAR_LINE) { nstate = InmosLink::RxState::RX_IDLE; next = 0; break; }

			// Otherwise, advance.
			next = current_time + m_event_step;
			break;
		}
		case InmosLink::RxState::RX_PACKETID:
		{
			// Is this link waiting for an ACK packet?
			if(m_links[link_num].waiting_for_ack)
			{
				// Yes, only clear the bit if we got an ACK.
				if(m_links[link_num].rx_value == InmosLink::PACKET_ACK)
				{
					device().logerror("link %d received ACK, advancing to IDLE.\n", link_num);
					m_links[link_num].waiting_for_ack = false;
					m_links[link_num].tx_state = InmosLink::TxState::TX_IDLE;
					got_ack_packet_cb();
				}

				// But always return to IDLE.
				nstate = InmosLink::RxState::RX_IDLE;
			}
			else
			{
				// No, we are expecting a data packet.

				// If this is an ACK packet we're not expecting, idle the receiver.
				if(m_links[link_num].rx_value == CLEAR_LINE) { nstate = InmosLink::RxState::RX_IDLE; next = 0; break; }
			}
			break;
		}
		case InmosLink::RxState::RX_DATABIT0:
		case InmosLink::RxState::RX_DATABIT1:
		case InmosLink::RxState::RX_DATABIT2:
		case InmosLink::RxState::RX_DATABIT3:
		case InmosLink::RxState::RX_DATABIT4:
		case InmosLink::RxState::RX_DATABIT5:
		case InmosLink::RxState::RX_DATABIT6:
		case InmosLink::RxState::RX_DATABIT7:
		{
			// Shift the incoming bit into the receive buffer.
			uint8_t bit = ((uint8_t)m_links[0].rx_value) << 7;
			m_links[link_num].rx_buffer >>= 1;
			m_links[link_num].rx_buffer |= bit;
			break;
		}
		case InmosLink::RxState::RX_STOPBIT:
		{
			// After 8 bits, expect a stop bit.

			// If stop bit is not 0, we lost synchronization.
			if(m_links[link_num].rx_value == ASSERT_LINE) { nstate = InmosLink::RxState::RX_IDLE; next = 0; break; }

			// We got a stop bit! That's the end of the data packet.

			// Callback: we received a byte packet on this link.
			received_full_byte_cb((InmosLink::LinkId)link_num);

			// Idle the receiver.
			nstate = InmosLink::RxState::RX_IDLE;
			next = 0;
			
			break;
		}
		default: break;
	}

	m_links[link_num].rx_next_event = next;
	m_links[link_num].rx_state = (InmosLink::RxState)nstate;
}

void device_inmos_serial_link_interface::serial_tx_update(u8 link_num, u64 current_time)
{
	u64	next = m_links[link_num].tx_next_event += m_event_step;
	u8 nstate = m_links[link_num].tx_state + 1;

	switch(m_links[link_num].tx_state)
	{
		case InmosLink::TxState::TX_IDLE:
		{
			next = 0; // idle
			nstate = InmosLink::TxState::TX_IDLE;
			break;
		}
		case InmosLink::TxState::TX_XMIT:
		{
			// Ready to transmit.
			next = current_time+1;
			break;
		}
		case InmosLink::TxState::TX_STARTBIT:
		{
			// Transmit the start bit, which is a 1.
			m_links[link_num].tx_value = ASSERT_LINE; m_links[link_num].tx_write_at_sync = true;

			break;
		}
		case InmosLink::TxState::TX_PACKETID:
		{
			// Packet ID is a 1.
			m_links[link_num].tx_value = ASSERT_LINE; m_links[link_num].tx_write_at_sync = true;

			break;
		}
		case InmosLink::TxState::TX_DATABIT0:
		case InmosLink::TxState::TX_DATABIT1:
		case InmosLink::TxState::TX_DATABIT2:
		case InmosLink::TxState::TX_DATABIT3:
		case InmosLink::TxState::TX_DATABIT4:
		case InmosLink::TxState::TX_DATABIT5:
		case InmosLink::TxState::TX_DATABIT6:
		case InmosLink::TxState::TX_DATABIT7:
		{
			// Send a data bit.

			m_links[link_num].tx_value = m_links[link_num].tx_buffer & 1; 	// Get LSB from buffer
			m_links[link_num].tx_buffer >>= 1; 								// Advance buffer one bit
			m_links[link_num].tx_write_at_sync = true;						// Ping the scheduler
			break;
		}
		case InmosLink::TxState::TX_STOPBIT:
		{
			// Stop bit is 0.
			m_links[link_num].tx_value = 0;
			m_links[link_num].tx_write_at_sync = true;

			// This is the end of a data packet. Wait for an ACK on this link.
			m_links[link_num].waiting_for_ack = true;
			break;			
		}
		case InmosLink::TxState::TX_WAITING_FOR_ACK:
		{
			// Link is waiting for an ACK packet.

			next = 0;
			nstate = InmosLink::TxState::TX_WAITING_FOR_ACK;
			break;
		}

		//////////////////////////////

		case InmosLink::TxState::TX_SENDING_ACK_STARTBIT:
		{
			// Send the acknowledge packet's start bit.
			m_links[link_num].tx_value = 1; m_links[link_num].tx_write_at_sync = true;
			nstate = InmosLink::TxState::TX_SENDING_ACK_ACKBIT;
			next = current_time + (m_event_step/2);
			break;
		}

		case InmosLink::TxState::TX_SENDING_ACK_ACKBIT:
		{
			// Send the acknowledge packet's ACK bit.
			m_links[link_num].tx_value = 0;
			m_links[link_num].tx_write_at_sync = true;

			// Idle the transmitter.
			nstate = InmosLink::TxState::TX_IDLE;
			next = 0;
			break;
		}

		default: break;
	}

	m_links[link_num].tx_next_event = next;
	m_links[link_num].tx_state = (InmosLink::TxState)nstate;
}

void device_inmos_serial_link_interface::perform_write_cb(s32 link_num)
{
	switch(link_num)
	{
		case 0: if(m_links[0].tx_write_at_sync) 
			{ link_out_w<0>(m_links[0].tx_value); m_links[0].tx_write_at_sync = false; if(m_links[link_num].tx_state == InmosLink::TxState::TX_IDLE) link_tx_is_ready(InmosLink::Link0); } break;
		case 1: if(m_links[1].tx_write_at_sync) { link_out_w<1>(m_links[1].tx_value); m_links[1].tx_write_at_sync = false; } break;
		case 2: if(m_links[2].tx_write_at_sync) { link_out_w<2>(m_links[2].tx_value); m_links[2].tx_write_at_sync = false; } break;
		case 3: if(m_links[3].tx_write_at_sync) { link_out_w<3>(m_links[3].tx_value); m_links[3].tx_write_at_sync = false; } break;
	}
}
