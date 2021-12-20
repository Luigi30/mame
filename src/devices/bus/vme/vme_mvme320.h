// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
#ifndef MAME_BUS_VME_VME_MVME320_H
#define MAME_BUS_VME_VME_MVME320_H

#pragma once

#include "cpu/8x300/8x300.h"
#include "imagedev/floppy.h"
#include "bus/vme/vme.h"
#include "bus/rs232/rs232.h"
#include "machine/clock.h"
#include "machine/mc68901.h"
#include "machine/ram.h"
#include "machine/8x371.h"
#include "machine/latch8.h"

DECLARE_DEVICE_TYPE(VME_MVME320, vme_mvme320_card_device)

//**************************************************************************
//  Base Device declaration
//**************************************************************************
class vme_mvme320_device :  public device_t, public device_vme_card_interface
{
public:
	vme_mvme320_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	auto in_sc_callback() { return m_in_sc_cb.bind(); }
	auto in_wc_callback() { return m_in_sc_cb.bind(); }
	auto in_lb_callback() { return m_in_lb_cb.bind(); }
	auto in_rb_callback() { return m_in_rb_cb.bind(); }
	auto in_iv_callback() { return m_in_iv_cb.bind(); }

protected:
	typedef enum {
		IO_BANK_NONE,
		IO_BANK_LB,
		IO_BANK_RB
	} IOBank;

	typedef enum
	{
		IO_PHASE_NONE,
		IO_PHASE_SC,
		IO_PHASE_WC
	} IOPhase;

	typedef enum
	{
		MCLK_ASSERTED,
		MCLK_CLEARED
	} MCLKState;

    required_device<n8x305_cpu_device> m_maincpu;
	required_device<floppy_connector> m_fd0;
	required_device<n8x371_device> m_dbcr;
	
	memory_passthrough_handler *m_state_machine_tap;
	void update_state_machine(offs_t offset, uint8_t data, uint8_t mem_mask);
	uint8_t m_state_machine;

	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;
    virtual const tiny_rom_entry *device_rom_region() const override;

    void device_add_mconfig(machine_config &config) override;
    void mvme320_program_map(address_map &map);
    void mvme320_io_map(address_map &map);

	void floppy_formats(format_registration &fr);

    void unscramble_proms();

	uint8_t registers_r(offs_t offset);
	void registers_w(offs_t offset, uint8_t data);

	uint8_t ram_bank_r(offs_t offset);
	void ram_bank_w(offs_t offset, uint8_t data);

	uint8_t buffer_u34_r();
	void buffer_u34_w(uint8_t data);
	uint8_t m_buffer_u34;

	uint8_t io_lb_r(offs_t offset);
	void io_lb_w(offs_t offset, uint8_t data);
	uint8_t io_rb_r(offs_t offset);
	void io_rb_w(offs_t offset, uint8_t data);

	devcb_read_line	m_in_sc_cb;
	devcb_read_line	m_in_wc_cb;
	devcb_read_line m_in_lb_cb;
	devcb_read_line m_in_rb_cb;
	devcb_read8 m_in_iv_cb;

	void lb_w(uint8_t data);
	void rb_w(uint8_t data);
	void sc_w(uint8_t data);
	void wc_w(uint8_t data);
	void mclk_w(uint8_t data);
	void iv_w(uint8_t data);

	IOBank m_io_bank;
	IOPhase m_io_phase;
	MCLKState m_mclk_state;
	uint8_t m_iv_state;

	uint32_t m_eca_address;
	uint8_t m_interrupt_vec;
	uint8_t	m_interrupt_src_status;
	uint8_t m_drive_status;

	bool m_watchdog;
	bool m_s0;

	// VME bus master stuff
	uint32_t m_vme_address;
	uint16_t m_vme_data;

	uint8_t m_vme_control_register;
	uint8_t m_vme_status_1, m_vme_status_2;

	uint8_t m_disk_control_1, m_disk_control_2, m_disk_control_3;
	uint8_t m_disk_buffer_address_latch;
	uint8_t m_disk_buffer[1024];

	uint8_t vme_read_lobyte();
	uint8_t vme_read_hibyte();
	void vme_write_byte();

	void update_vme_control_lines(uint8_t data);

	emu_timer *m_us_to_vme_read_timer;	// 0
	emu_timer *m_us_to_vme_write_timer;	// 1

	emu_timer *m_vme_to_us_read_timer; 	// 2
	emu_timer *m_vme_to_us_write_timer;	// 3

	typedef enum
	{
		VSR1n,		// VME Status Register 1
		RDBCn,		// Read Disk Bit Controller
		VRDLn,		// VME Read Data Lower
		RBUn,		// Read Buffer
		VSR2n,		// VME Status Register 2
		RDSn,		// Read Disk Status
		VRDUn,		// VME Read Data Upper
		NOP			// No operation
	} ReadState;

	typedef enum
	{
		NOP0,
		WUASn,		// Write Upper Address Strobe
		WUDSn,		// Write Upper Data Strobe
		WRDn,		// Write Register $D
		WLDSn,		// Write Lower Data Strobe
		VCRn,		// VME Control Register
		WMASn,		// Write Middle Address Strobe
		WLASn,		// Write Lower Address Strobe
		WDC1n,		// Write Disk Control 1
		WDBCn,		// Write Disk Bit Control
		WDC3n,		// Write Disk Control 3
		NOP11,
		WDC2n,		// Write Disk Control 2
		NOP13,	
		WBUn,		// Write Buffer
		NOP15
	} WriteState;

	ReadState m_read_state;
	WriteState m_write_state;
	bool m_read_line_state[8];
	bool m_write_line_state[16];

	// write lines from the decoder
	void decoder_write_outputs();
	void WUASn_w(bool state);
	void WUDSn_w(bool state);
	void WRDn_w(bool state);
	void WLDSn_w(bool state);
	void VCRn_w(bool state);
	void WMASn_w(bool state);
	void WLASn_w(bool state);
	void WDC1n_w(bool state);
	void WDBCn_w(bool state);
	void WDC3n_w(bool state);
	void NOP11_w(bool state);
	void WDC2n_w(bool state);
	void NOP13_w(bool state);
	void WBUn_w(bool state);
	void NOP15_w(bool state);

	// read lines from the decoder
	void decoder_read_outputs();
	void VSR1n_w(bool state);
	void RDBCn_w(bool state);
	void VRDLn_w(bool state);
	void RBUn_w(bool state);
	void VSR2n_w(bool state);
	void RDSn_w(bool state);
	void VRDUn_w(bool state);
};

//**************************************************************************
//  Board Device declarations
//**************************************************************************

class vme_mvme320_card_device : public vme_mvme320_device
{
public:
	vme_mvme320_card_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	vme_mvme320_card_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock)
		: vme_mvme320_device(mconfig, type, tag, owner, clock)
	{ }
};

#endif // MAME_BUS_VME_VME_MVME320_H