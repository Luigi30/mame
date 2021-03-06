// license:BSD-3-Clause
// copyright-holders:Aaron Giles
#ifndef MAME_MACHINE_NAMCO50_H
#define MAME_MACHINE_NAMCO50_H

#pragma once

#include "cpu/mb88xx/mb88xx.h"

/* device get info callback */
class namco_50xx_device : public device_t
{
public:
	namco_50xx_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	void write(uint8_t data);
	WRITE_LINE_MEMBER(read_request);
	uint8_t read();

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual const tiny_rom_entry *device_rom_region() const override;
	virtual void device_add_mconfig(machine_config &config) override;

	TIMER_CALLBACK_MEMBER( latch_callback );
	TIMER_CALLBACK_MEMBER( readrequest_callback );
	TIMER_CALLBACK_MEMBER( irq_clear );
	void irq_set();

private:
	// internal state
	required_device<mb88_cpu_device> m_cpu;
	uint8_t                   m_latched_cmd;
	uint8_t                   m_latched_rw;
	uint8_t                   m_portO;
	emu_timer * m_irq_cleared_timer;

	uint8_t K_r();
	uint8_t R0_r();
	uint8_t R2_r();
	void O_w(uint8_t data);
};

DECLARE_DEVICE_TYPE(NAMCO_50XX, namco_50xx_device)

#endif // MAME_MACHINE_NAMCO50_H
