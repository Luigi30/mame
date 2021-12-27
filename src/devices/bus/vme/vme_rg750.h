// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
#ifndef MAME_BUS_VME_VME_MVME050_H
#define MAME_BUS_VME_VME_MVME050_H

#pragma once

#include "bus/vme/vme.h"
#include "cpu/tms34010/tms34010.h"
#include "machine/clock.h"
#include "video/bt47x.h"
#include "screen.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

DECLARE_DEVICE_TYPE(VME_RG750,   vme_rg750_card_device)

//**************************************************************************
//  Base Device declaration
//**************************************************************************
class vme_rg750_device :  public device_t, public device_vme_card_interface
{
public:
	/* Board types */
	vme_rg750_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);
	
protected:
	void device_add_mconfig(machine_config &config) override;
	virtual const tiny_rom_entry *device_rom_region() const override;
	virtual ioport_constructor device_input_ports() const override;
	
	virtual void device_start () override;
	virtual void device_reset () override;
	
	void rg750_mem(address_map &map);
	
	required_device<tms34010_device> m_maincpu;
	required_device<screen_device> m_screen;
	optional_device<bt478_device> m_vdac;

	required_shared_ptr<uint16_t> m_vram;
	required_shared_ptr<uint16_t> m_dram;
	
	uint8_t		m_ctrlreg;
	uint8_t		m_statusreg;
	
	uint8_t 	ctrlreg_r(offs_t offset);
	void 		ctrlreg_w(offs_t offset, uint8_t data);
	
	uint8_t 	statusreg_r(offs_t offset);
	void 		statusreg_w(offs_t offset, uint8_t data);

	void 		screen_update(screen_device &screen, bitmap_rgb32 &bitmap, rectangle const &cliprect);

	TMS340X0_TO_SHIFTREG_CB_MEMBER(to_shiftreg);
	TMS340X0_FROM_SHIFTREG_CB_MEMBER(from_shiftreg);
	TMS340X0_SCANLINE_IND16_CB_MEMBER(scanline_update);
};


//**************************************************************************
//  Board Device declarations
//**************************************************************************

class vme_rg750_card_device : public vme_rg750_device
{
public :
	vme_rg750_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	vme_rg750_card_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
		: vme_rg750_device(mconfig, type, tag, owner, clock)
	{ }
};

#endif // MAME_BUS_VME_VME_MVME050_H
