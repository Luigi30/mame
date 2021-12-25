// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
#ifndef MAME_BUS_VME_VME_MVME120_H
#define MAME_BUS_VME_VME_MVME120_H

#pragma once

#include "cpu/m68000/m68000.h"
#include "bus/vme/vme.h"
#include "bus/rs232/rs232.h"
#include "machine/clock.h"
#include "machine/mc68901.h"

DECLARE_DEVICE_TYPE(VME_MVME120,   vme_mvme120_card_device)
DECLARE_DEVICE_TYPE(VME_MVME121,   vme_mvme121_card_device)
DECLARE_DEVICE_TYPE(VME_MVME122,   vme_mvme122_card_device)
DECLARE_DEVICE_TYPE(VME_MVME123,   vme_mvme123_card_device)

//**************************************************************************
//  Base Device declaration
//**************************************************************************
class vme_mvme120_device :  public device_t, public device_vme_card_interface
{
public:
	/* Board types */
	enum mvme12x_variant
	{
		mvme120_board,
		mvme121_board,
		mvme122_board,
		mvme123_board
	};

	vme_mvme120_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock, mvme12x_variant board_id);

	// Switch and jumper handlers
	DECLARE_INPUT_CHANGED_MEMBER(s3_autoboot);
	DECLARE_INPUT_CHANGED_MEMBER(s3_baudrate);
	
	DECLARE_WRITE_LINE_MEMBER( vme_bus_error_changed );
			
	uint16_t vme_to_ram16_r(offs_t offset);
	void vme_to_ram16_w(offs_t address, uint16_t data);

	uint8_t vme_to_ram8_r(offs_t offset);
	void vme_to_ram8_w(offs_t address, uint8_t data);

protected:
	void device_add_mconfig(machine_config &config) override;
	virtual const tiny_rom_entry *device_rom_region() const override;
	virtual ioport_constructor device_input_ports() const override;

	virtual void device_start() override;
	virtual void device_reset() override;

	void mvme12x_base_mem(address_map &map);
	void mvme120_mem(address_map &map);
	void mvme121_mem(address_map &map);
	void mvme122_mem(address_map &map);
	void mvme123_mem(address_map &map);

	required_device<cpu_device> m_maincpu;

	required_device<mc68901_device> m_mfp;
	required_device<rs232_port_device> m_rs232;

	required_ioport m_input_s3;

	memory_passthrough_handler *m_rom_shadow_tap;
	memory_passthrough_handler *m_ram_parity_tap;

	required_region_ptr<uint16_t> m_sysrom;
	required_shared_ptr<uint16_t> m_localram;

	uint8_t     m_ctrlreg;              // "VME120 Control Register"
	uint8_t     m_memory_read_count;    // For boot ROM shadowing $000000

	uint8_t     ctrlreg_r(offs_t offset);
	void        ctrlreg_w(offs_t offset, uint8_t data);

	// VMEbus dummy lines
	void vme_bus_timeout();
	uint16_t vme_a24_r(offs_t offset);
	void vme_a24_w(offs_t offset, uint16_t data);
	uint8_t vme_a16_r(offs_t offset);
	void vme_a16_w(offs_t offset, uint8_t data);

	uint16_t rom_shadow_tap(offs_t address, u16 data, u16 mem_mask);
	uint16_t ram_parity_tap_r(offs_t address, u16 data, u16 mem_mask);
	void ram_parity_tap_w(offs_t address, u16 data, u16 mem_mask);

	DECLARE_WRITE_LINE_MEMBER(watchdog_reset);
	DECLARE_WRITE_LINE_MEMBER(mfp_interrupt);

	const mvme12x_variant  m_board_id;

	virtual void vme_berr_w(int state) override;
	virtual void vme_dtack_w(int state) override;

	emu_timer *m_vme_bus_timeout_timer;
	TIMER_CALLBACK_MEMBER(vme_bus_timeout_expired
	);
	int m_vme_dtack_trigger;
	bool m_performing_vme_transaction;
};


//**************************************************************************
//  Board Device declarations
//**************************************************************************

class vme_mvme120_card_device : public vme_mvme120_device
{
public:
	vme_mvme120_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	vme_mvme120_card_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme120_device(mconfig, type, tag, owner, clock, mvme120_board)
	{ }

	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

class vme_mvme121_card_device : public vme_mvme120_device
{
public:
	vme_mvme121_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	vme_mvme121_card_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme120_device(mconfig, type, tag, owner, clock, mvme121_board)
	{ }

	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};


class vme_mvme122_card_device : public vme_mvme120_device
{
public:
	vme_mvme122_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	vme_mvme122_card_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme120_device(mconfig, type, tag, owner, clock, mvme122_board)
	{ }

	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};


class vme_mvme123_card_device : public vme_mvme120_device
{
public:
	vme_mvme123_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	vme_mvme123_card_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme120_device(mconfig, type, tag, owner, clock, mvme123_board)
	{ }

	// optional information overrides
	virtual void device_add_mconfig(machine_config &config) override;
};

#endif // MAME_BUS_VME_VME_MVME120_H
