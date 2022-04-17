// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

#ifndef MAME_BUS_INMOS_LINK_H
#define MAME_BUS_INMOS_LINK_H

// INMOS serial link.
// 8 data bits, no parity, 1 stop bit.
// Normal speed is 10Mbit/sec.
//
// An INMOS link is a two-wire interface with in-band signalling.
// The transmitter sends 1 start bit, a logical 1 bit, 8 data bytes, 1 stop bit.
// When the receiver is ready for the next byte, it acknowledges with 1 start bit and a logical 0 bit.
//

namespace InmosLink
{
	typedef enum
	{
		Link0,
		Link1,
		Link2,
		Link3
	} LinkId;

	enum
	{
		PACKET_ACK,
		PACKET_BYTE
	};

	typedef enum
	{
		TX_IDLE,				// Idle, transmit buffer is empty.
		TX_XMIT,				// Just got a byte, beginning transmission.
		TX_STARTBIT,      		// 1 start bit
		TX_PACKETID,      		// 1 for data packet, 0 for acknowledge packet
		TX_DATABIT0,      		// data bit 0, LSB
		TX_DATABIT1,			// data bit 1
		TX_DATABIT2,			// data bit 2
		TX_DATABIT3,			// data bit 3
		TX_DATABIT4,			// data bit 4
		TX_DATABIT5,			// data bit 5
		TX_DATABIT6,			// data bit 6
		TX_DATABIT7,			// data bit 7
		TX_STOPBIT,         	// 1 stop bit
		TX_WAITING_FOR_ACK,		// waiting for ACK packet

		TX_SENDING_ACK_STARTBIT,
		TX_SENDING_ACK_ACKBIT
	} TxState;

	typedef enum
	{
		RX_IDLE,			// Idle, waiting for transmission.
		RX_STARTBIT,		// Got a start bit.
		RX_PACKETID,		// 1 for data packet, 0 for acknowledge packet
		RX_DATABIT0,		// Got data bit 0.
		RX_DATABIT1,		// Got data bit 1.
		RX_DATABIT2,		// Got data bit 2.
		RX_DATABIT3,		// Got data bit 3.
		RX_DATABIT4,		// Got data bit 4.
		RX_DATABIT5,		// Got data bit 5.
		RX_DATABIT6,		// Got data bit 6.
		RX_DATABIT7,		// Got data bit 7.
		RX_STOPBIT,			// 1 stop bit
		RX_ACK_STARTBIT,	// sending ACK: 1 start bit
		RX_ACK_ACKBIT		// sending ACK: logical 0
	} RxState;

	std::string RxState_to_String(RxState rxstate);
	std::string TxState_to_String(TxState txstate);

	typedef struct
	{
		bool		waiting_for_ack;	// Set when this link is holding on receiving an ACK packet.

		TxState 	tx_state;			// The state of the Tx state machine.
		uint8_t		tx_buffer;			// The byte being transmitted.
		u64			tx_next_event;		// The cycle of the next Tx event.
		bool		tx_value;			// The state of this link's Out line.
		bool		tx_write_at_sync;	// Set when the next synchronize will result in a bit being sent.

		RxState		rx_state;			// The state of the Rx state machine.
		uint8_t		rx_buffer;			// The byte being received.
		u64			rx_next_event;		// The cycle of the next Rx event.
		bool		rx_value;			// The state of this link's In line.
		bool		rx_byte_present;	// Set when the next synchronize will result in a bit being recieved.

		bool tx_is_ready() { return (tx_state == InmosLink::TxState::TX_IDLE && waiting_for_ack == false && tx_write_at_sync == false); }

		void reset()
		{
			waiting_for_ack = false;

			tx_state = TxState::TX_IDLE;
			tx_buffer = 0;
			tx_next_event = 0;
			tx_value = 0;
			tx_write_at_sync = 0;

			rx_state = RxState::RX_IDLE;
			rx_buffer = 0;
			rx_next_event = 0;
			rx_value = 0;
			rx_byte_present = false;
		}

		void send_ack_packet(u64 current_time)
		{
			tx_next_event = current_time;
			tx_state = InmosLink::TxState::TX_SENDING_ACK_STARTBIT;
		}

		void send_byte(u8 byte, u64 next_edge)
		{
			tx_buffer = byte;
			tx_next_event = next_edge;
			tx_state = InmosLink::TxState::TX_XMIT;
		}
	} LinkInfo;
};

class device_inmos_serial_link_interface : public device_interface
{
	public:
	device_inmos_serial_link_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device, "inmos_serial_link")
	, m_link_out_cb(*this)
	{
	}

	template<offs_t i> auto link_out_cb() 			{ return m_link_out_cb[i].bind(); }
	template<offs_t i> auto link_out_w(bool value) 	{ m_links[i].tx_value = value; m_link_out_cb[i](value); }

	// Write lines for receiving a bit on each possible link.
	virtual DECLARE_WRITE_LINE_MEMBER(link_in0_w) = 0;
	virtual DECLARE_WRITE_LINE_MEMBER(link_in1_w) = 0;
	virtual DECLARE_WRITE_LINE_MEMBER(link_in2_w) = 0;
	virtual DECLARE_WRITE_LINE_MEMBER(link_in3_w) = 0;

	//
	// Callbacks implemented by devices that have INMOS links.
	//
	// Callback: The device received a full one-byte data packet.
	virtual void received_full_byte_cb(InmosLink::LinkId link) = 0;
	// Callback: The device is sensing the state of its serial links.
	virtual void perform_read_cb(s32 param) = 0;
	// Callback: The device got an ACK packet on a link.
	virtual void got_ack_packet_cb() = 0;

	// Called by the scheduler().synchronize() function.
	void perform_write_cb(s32 param);

	protected:
	// Update the serial link state machine. Called by a device during its run loop.
	virtual void internal_update(uint64_t current_time) = 0;

	// Rx and Tx state machine processing.
	void serial_rx_update(u8 link_num, u64 current_time);
	void serial_tx_update(u8 link_num, u64 current_time);

	uint64_t					m_event_step;
	InmosLink::LinkInfo 		m_links[4];
	devcb_write_line::array<4>  m_link_out_cb;

	private:

};

#endif