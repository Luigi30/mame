// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
#ifndef MAME_BUS_INMOS_B011_H
#define MAME_BUS_INMOS_B011_H

#pragma once

#include "inmos.h"
#include "cpu/inmos/t2.h"
#include "cpu/inmos/t8.h"

class ims_b411_device
	: public device_t
	, public device_inmos_tram_interface
{
	public:
	ims_b411_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

	virtual void inmos0_in_w(uint8_t state) override;
	virtual void inmos0_out_w(uint8_t state) override;

	virtual WRITE_LINE_MEMBER(analyse_w) override 	{ m_cpu->analyse_w(state); }
	virtual WRITE_LINE_MEMBER(reset_w) override 	{ m_cpu->reset_w(state); }

	protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_add_mconfig(machine_config &config) override;

	private:
	required_device<t800_cpu_device> m_cpu;

	void cpu_map(address_map &map);
	void crossbar_mcu_map(address_map &map);
};

DECLARE_DEVICE_TYPE(IMS_B411, ims_b411_device)

#endif // MAME_BUS_INMOS_B011_H