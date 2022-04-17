// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

#ifndef MAME_BUS_INMOS_INMOS_H
#define MAME_BUS_INMOS_INMOS_H

/*
	INMOS TRAM 

	Slot pinout:
	1   Link2Out        Link3In     16
	2   Link2In         Link3Out    15
	3   VCC             GND         14
	4   Link1Out        Link0In     13
	5   Link1In         Link0Out    12
	6   LinkSpeedA      NotError    11
	7   LinkSpeedB      Reset       10
	8   ClockIn         Analyse     9

	Subsystem port (optional)
	1a  SubsystemReset
	2a  SubsystemAnalyse
	3a  SubsystemNotError
*/

class inmos_slot_device;
class device_inmos_tram_interface;

#include "bus/inmos/link.h"

DECLARE_DEVICE_TYPE(INMOS_SLOT, inmos_slot_device)

class inmos_slot_device : public device_t, 
						  public device_single_card_slot_interface<device_inmos_tram_interface>
{
	public:
	template <typename T, typename U>
	inmos_slot_device(machine_config const &mconfig, char const *tag, device_t *owner, T &&opts, char const *dflt, uint32_t slot_nbr, U &&bus_tag)
		: inmos_slot_device(mconfig, INMOS_SLOT, tag, owner, 0)
	{
		option_reset();
		opts(*this);
		set_default_option(dflt);
		set_fixed(false);
		set_inmos_slot(std::forward<U>(bus_tag), slot_nbr);
	}
	inmos_slot_device(machine_config const &mconfig, char const *tag, device_t *owner, uint32_t clock);

	template <typename T> void set_inmos_slot(T &&tag, uint32_t slot_nbr) { m_slot_nbr = slot_nbr; }

	protected:
	inmos_slot_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	virtual void device_start() override 
	{
	}

	private:
	uint32_t m_slot_nbr;
};

/*********************/

// TRAMs always have 4 links.
class device_inmos_tram_interface : public device_interface
{
	public:
	virtual void inmos0_in_w(uint8_t state) {}
	virtual void inmos0_out_w(uint8_t state) {}

	// Link and System Services lines

	// Output: Link and Error
	template<offs_t i>  auto link_out_cb()              { return m_link_out_cb[i].bind(); }
						auto error_cb()                 { return m_error_cb.bind(); }

	template<offs_t i>  auto link_out_w(uint8_t value)  { m_link_out_cb[i](value); }
						auto error_w(uint8_t value)     { m_error_cb(value); }

	// Input: Analyse and Reset
	virtual DECLARE_WRITE_LINE_MEMBER(analyse_w) 	{}
	virtual DECLARE_WRITE_LINE_MEMBER(reset_w) 		{}

	protected:
		friend class inmos_slot_device;

		device_inmos_tram_interface(const machine_config &mconfig, device_t &device);

		// device_interface implementation
		void interface_pre_start() override ATTR_COLD;

		devcb_write_line::array<4>  m_link_out_cb;
		devcb_write_line            m_error_cb;

	private:
		unsigned m_index;
};

/**********************/

void inmos_cards(device_slot_interface &device);

#endif