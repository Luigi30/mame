// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

    INMOS C012 Link Adapter emulation

	Contains one INMOS link.

	The C012 links a serial transputer bus to a parallel host bus.
	In the B008, this connects the ISA bus and Slot0/Link0.

	I/O is buffered but the datasheet doesn't specify how big the buffer is.

	Terminology:
	- "Input" is always serial link -> parallel bus
	- "Output" is always parallel bus -> serial link

	1	LinkOut			VCC			24
	2	LinkIn			CapMinus	23
	3	RnotW			InputInt	22
	4	OutputInt		notCS		21
	5	RS0				D0			20
	6	RS1				D1			19
	7	D3				D2			18
	8	D5				D4			17
	9	HoldToGND		D6			16
	10	D7				LinkSpeed	15
	11	Reset			HoldToGND	14
	12	GND				ClockIn		13

***************************************************************************/

#ifndef MAME_MACHINE_IMSC012_H
#define MAME_MACHINE_IMSC012_H

#pragma once

#include "bus/inmos/inmos.h"

class ims_c012_device : 
	public device_t, 
	public device_execute_interface,
	public device_inmos_serial_link_interface
{
    public:
	// construction/destruction
	ims_c012_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);    

	// Register accessors
	uint8_t input_r();
	uint8_t output_r();
	void output_w(uint8_t data);

	uint8_t input_status_r();
	void input_status_w(uint8_t data);
	uint8_t output_status_r();
	void output_status_w(uint8_t data);

	// Signal lines
	auto input_int_write() { return m_input_int_func.bind(); }
	auto output_int_write() { return m_output_int_func.bind(); }

	DECLARE_WRITE_LINE_MEMBER(rx0_w) { serial_rx_w(0, state); }
	auto tx0_handler() { return m_tx_cb[0].bind(); }

	virtual DECLARE_WRITE_LINE_MEMBER(link_in0_w) override;
	virtual WRITE_LINE_MEMBER(link_in1_w) override {}
	virtual WRITE_LINE_MEMBER(link_in2_w) override {}
	virtual WRITE_LINE_MEMBER(link_in3_w) override {}

    protected:
	ims_c012_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	devcb_write_line m_tx_cb[1];

	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_resolve_objects() override;
	virtual void execute_run() override;

	virtual void perform_read_cb(s32 param) override;
	virtual void got_ack_packet_cb() override;
	virtual void received_full_byte_cb(InmosLink::LinkId link) override;
	virtual void link_tx_is_ready(InmosLink::LinkId link) override;
	virtual void internal_update(uint64_t current_time = 0) override;
	void recompute_bcount(uint64_t event_time);		
	void serial_rx_w(int ch, int state);
	u64	serial_get_next_edge();

	private:
	// Signal lines for interrupts.
	devcb_write_line m_input_int_func;
	devcb_write_line m_output_int_func;

	// Internal registers.
	uint8_t m_input_data;	// transputer -> host
	uint8_t m_output_data;	// host -> transputer

	bool m_input_interrupt_enable;
	bool m_output_interrupt_enable;
	bool m_input_data_present;
	bool m_output_ready;

	int m_icount, m_bcount;
};

DECLARE_DEVICE_TYPE(IMS_C012, ims_c012_device)

#endif