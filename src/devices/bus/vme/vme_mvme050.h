// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
#ifndef MAME_BUS_VME_VME_MVME050_H
#define MAME_BUS_VME_VME_MVME050_H

#pragma once

#include "bus/centronics/ctronics.h"
#include "bus/rs232/rs232.h"
#include "bus/vme/vme.h"
#include "cpu/m68000/m68000.h"
#include "machine/68153bim.h"
#include "machine/68561mpcc.h"
#include "machine/clock.h"
#include "machine/mc146818.h"

DECLARE_DEVICE_TYPE(VME_MVME050,   vme_mvme050_card_device)

//**************************************************************************
//  Base Device declaration
//**************************************************************************
class vme_mvme050_device :  public device_t, public device_vme_card_interface
{
public:
	/* Board types */
	vme_mvme050_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);
	
protected:
	void device_add_mconfig(machine_config &config) override;
	virtual const tiny_rom_entry *device_rom_region() const override;
	virtual ioport_constructor device_input_ports() const override;
	
	virtual void device_start () override;
	virtual void device_reset () override;
	
	void mvme050_mem(address_map &map);
	
	required_device<mc146818_device> 		m_rtc;
	required_device<mpcc68561_device> 		m_mpcc1;
	required_device<mpcc68561_device> 		m_mpcc2;
	required_device<bim68153_device> 		m_bim1;
	required_device<bim68153_device> 		m_bim2;

	required_device<centronics_device>		m_lpt;
	required_device<output_latch_device> 	m_lpt_latch;

	// VMEbus handling
	uint16_t 	unmapped_vme_r(address_space &space, offs_t offset, uint16_t mem_mask);
	void		unmapped_vme_w(address_space &space, offs_t address, uint16_t data, uint16_t mem_mask);

	// Front panel
	uint8_t		frontpanel_r(address_space &space, offs_t offset, uint8_t mem_mask);
	void		frontpanel_w(address_space &space, offs_t offset, uint8_t data, uint8_t mem_mask);

	uint8_t		m_frontpanel_value;
	bool		m_frontpanel_blanked;

	// BIM
	uint8_t 	bim1_irq_state;
	int 		bim1_irq_level;

	uint8_t 	bim2_irq_state;
	int 		bim2_irq_level;

	void        update_irq_to_maincpu();
	void		set_frontpanel_blanking(bool state);

	DECLARE_WRITE_LINE_MEMBER(bim1_irq_callback);
	DECLARE_WRITE_LINE_MEMBER(bim2_irq_callback);

	// Printer
	uint8_t		printer_r(address_space &space, offs_t offset, uint8_t mem_mask);
	void		printer_w(address_space &space, offs_t offset, uint8_t data, uint8_t mem_mask);

	uint8_t  	m_printer_ack_in;
	uint8_t  	m_printer_perror_in;
	uint8_t  	m_printer_select_in; 
	uint8_t  	m_printer_fault_in;
	DECLARE_WRITE_LINE_MEMBER(printer_ack_w);
	DECLARE_WRITE_LINE_MEMBER(printer_perror_w);
	DECLARE_WRITE_LINE_MEMBER(printer_select_w);
	DECLARE_WRITE_LINE_MEMBER(printer_fault_w);

	virtual void vme_sysfail_w(int state) override;	
};

//**************************************************************************
//  Board Device declarations
//**************************************************************************

class vme_mvme050_card_device : public vme_mvme050_device
{
public :
	vme_mvme050_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	vme_mvme050_card_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme050_device(mconfig, type, tag, owner, clock)
	{ }
};

#endif // MAME_BUS_VME_VME_MVME050_H
