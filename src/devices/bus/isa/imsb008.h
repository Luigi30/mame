// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
#ifndef MAME_BUS_ISA_IMSB008_H
#define MAME_BUS_ISA_IMSB008_H

#pragma once

#include "isa.h"
#include "bus/inmos/inmos.h"
#include "machine/imsc012.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class isa16_imsb008_device : public device_t,
	public device_isa16_card_interface
{
	public:
	// construction/destruction
	isa16_imsb008_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	void error_in_w(uint8_t value) { m_error = (value > 0); }

	protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_add_mconfig(machine_config &config) override;
	virtual void device_resolve_objects() override;
	virtual ioport_constructor device_input_ports() const override;

	// MMIO registers.
	uint8_t analyse_register_r();
	uint8_t error_register_r();
	uint8_t dma_request_register_r();
	uint8_t irq_control_register_r();

	void reset_register_w(uint8_t value);
	void analyse_register_w(uint8_t value);
	void dma_request_register_w(uint8_t value);
	void irq_control_register_w(uint8_t value);

	required_ioport m_sw1;

	private:
	void map(address_map &map);

	required_device<inmos_slot_device> m_slot0;
	required_device<ims_c012_device> m_link_adapter;

	device_inmos_tram_interface *m_trams[10];

	bool m_error;
};

// device type definition
DECLARE_DEVICE_TYPE(ISA16_IMSB008, isa16_imsb008_device)

#endif // MAME_BUS_ISA_IDE_H