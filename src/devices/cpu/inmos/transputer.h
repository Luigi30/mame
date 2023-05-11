// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

	transputer.h
	Header for the INMOS transputer architecture.

***************************************************************************/

#ifndef MAME_CPU_INMOS_TRANSPUTER_H
#define MAME_CPU_INMOS_TRANSPUTER_H

#pragma once

#include "machine/ram.h"
#include "transputer_private.h"
#include "bus/inmos/inmos.h"

class transputer_cpu_device : public cpu_device, public device_inmos_serial_link_interface
{
	public:
	enum
	{
		TRANSPUTER_IPTR, TRANSPUTER_WDESC, 
		TRANSPUTER_FPTR0, TRANSPUTER_BPTR0,
		TRANSPUTER_FPTR1, TRANSPUTER_BPTR1,
		TRANSPUTER_AREG, TRANSPUTER_BREG, TRANSPUTER_CREG, TRANSPUTER_OREG,
		TRANSPUTER_TIME0, TRANSPUTER_TIME1,
		BOOTSTRAP_STATE, RUN_STATE
	};

	typedef enum {
		LINK_OUTPUT_0   = 0x80000000,   // MMIO address for sending on link 0.
		LINK_OUTPUT_1   = 0x80000004,   // MMIO address for sending on link 1.
		LINK_OUTPUT_2   = 0x80000008,   // MMIO address for sending on link 2.
		LINK_OUTPUT_3   = 0x8000000C,   // MMIO address for sending on link 3.
		LINK_INPUT_0    = 0x80000010,   // MMIO address for receiving on link 0.
		LINK_INPUT_1    = 0x80000014,   // MMIO address for receiving on link 1.
		LINK_INPUT_2    = 0x80000018,   // MMIO address for receiving on link 2.
		LINK_INPUT_3    = 0x8000001C,   // MMIO address for receiving on link 3.
	} TRANSPUTER_LINK;

	typedef enum
	{
		CPU_WAITING_FOR_BOOTSTRAP = 0,
		CPU_RUNNING = 1
	} CPU_STATE;

	transputer_cpu_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock, u32 word_width);

	DECLARE_WRITE_LINE_MEMBER(reset_w);
	DECLARE_WRITE_LINE_MEMBER(analyse_w);
	DECLARE_WRITE_LINE_MEMBER(boot_from_rom_w);

	virtual void internal_map(address_map &map);

	// TODO: These need to be Protected and internally mapped
	uint32_t TPtrLoc0_r();
	uint32_t TPtrLoc1_r();
	uint32_t Event_r();
	uint32_t WdescIntSaveLoc_r();
	uint32_t IptrIntSaveLoc_r();
	uint32_t AregIntSaveLoc_r();
	uint32_t BregIntSaveLoc_r();
	uint32_t CregIntSaveLoc_r();
	uint32_t STATUSIntSaveLoc_r();
	uint32_t EregIntSaveLoc_r();

	void TPtrLoc0_w(uint32_t value);
	void TPtrLoc1_w(uint32_t value);
	void Event_w(uint32_t value);
	void WdescIntSaveLoc_w(uint32_t value);
	void IptrIntSaveLoc_w(uint32_t value);
	void AregIntSaveLoc_w(uint32_t value);
	void BregIntSaveLoc_w(uint32_t value);
	void CregIntSaveLoc_w(uint32_t value);
	void STATUSIntSaveLoc_w(uint32_t value);
	void EregIntSaveLoc_w(uint32_t value);

	virtual DECLARE_WRITE_LINE_MEMBER(link_in0_w) override;
	virtual DECLARE_WRITE_LINE_MEMBER(link_in1_w) override;
	virtual DECLARE_WRITE_LINE_MEMBER(link_in2_w) override;
	virtual DECLARE_WRITE_LINE_MEMBER(link_in3_w) override;
	virtual void received_full_byte_cb(InmosLink::LinkId link) override;

	auto error_cb() { return m_error_cb.bind(); }
	auto error_w(uint8_t value) { m_error_cb(value); }

	void set_error_flag()   { m_errorFlag = true; m_error_cb(m_errorFlag); }
	void clear_error_flag() { m_errorFlag = false; m_error_cb(m_errorFlag); }

	///////////////////////////////////////////////////////////////////////////////////
	protected:
	transputer_cpu_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock, int cpu_type, u32 word_width);

	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_add_mconfig(machine_config &config) override;
	virtual void device_resolve_objects() override;

	// device_execute_interface overrides
	virtual u32 execute_min_cycles() const noexcept override { return 1; }
	virtual u32 execute_max_cycles() const noexcept override { return 48; }
	virtual u32 execute_input_lines() const noexcept override { return 0; }
	virtual void execute_run() override;

	// device_memory_interface overrides
	virtual space_config_vector memory_space_config() const override;

	// device_disasm_interface overrides
	virtual std::unique_ptr<util::disasm_interface> create_disassembler() override;

	// INMOS link interface
	virtual void perform_read_cb(s32 param) override;
	virtual void internal_update(uint64_t current_time = 0) override;
	virtual void got_ack_packet_cb() override;
	virtual void link_tx_is_ready(InmosLink::LinkId link) override;
	void recompute_bcount(uint64_t event_time);
	u64 serial_get_next_edge();

	// bootstrap functions
	virtual void bootstrap_step();
	virtual void bootstrap_begin_execution();

	address_space_config m_program_config;
	memory_access<32, 2, 0, ENDIANNESS_LITTLE>::specific memory;

	u32 m_IPTR;                 // Instruction Pointer
	u32 m_WDESC;                // Workspace Descriptor
	u32 m_AREG, m_BREG, m_CREG; // A, B, C Registers
	u32 m_OREG;                 // Operand Register

	u32 m_FPTR0;                // High priority process list, front.
	u32 m_BPTR0;                // High priority process list, back.
	u32 m_FPTR1;                // Low priority process list, front.
	u32 m_BPTR1;                // Low priority process list, back.

	uint32_t m_OReg0;           // Temp for instruction decoding.

	u32 m_ClockReg0;            // High-priority timer, counts 1uS ticks.
	u32 m_ClockReg1;            // Low-priority timer, counts 64uS ticks.

	enum
	{
		CPUTYPE_T212,
		CPUTYPE_T414,
		CPUTYPE_T800
	};

	uint8_t fetch();

	int DoOperation();

	// The address of the next instruction. TODO: What changes this from 1?
	inline uint32_t OP_NextInst()   { return m_IPTR + 1; }

	inline void     OP_StackPush(uint32_t value)  { m_CREG = m_BREG; m_BREG = m_AREG; m_AREG = value; }
	// Pop the A-B-C stack. C is undefined per documentation but real hardware leaves it unchanged.
	inline u32      OP_StackPop()                 { u32 tmp = m_AREG; m_AREG = m_BREG; m_BREG = m_CREG; return tmp; }
	// Advance the Iptr.
	inline void     OP_IptrAdvance()              { m_IPTR = OP_NextInst(); }

	// Read a word from Workspace.
	inline uint32_t OP_ReadWordFromWorkspace(uint32_t offset) { return memory.read_dword(Wptr + (offset*4)); }
	// Write a word to Workspace.
	inline void     OP_WriteWordToWorkspace(uint32_t offset, uint32_t Areg) { memory.write_dword(Wptr + (offset*4), Areg); }
	inline void     OP_ClearOreg() { m_OREG = 0; }
	inline void     OP_UpdateWptr(uint32_t value) { m_WDESC = (m_WDESC & 3); m_WDESC |= value; }
	inline uint32_t OP_MaskByteSelector(uint32_t value) { return value & WORDMASK; }
	inline uint32_t OP_Index(uint32_t x, uint32_t y) { return (x + (BYTESPERWORD*y)); }
	inline uint32_t OP_ByteIndex(uint32_t x, uint32_t y) { return x + y; }

	void link0_ccw_w(uint8_t data);
	void link1_ccw_w(uint8_t data);
	void link2_ccw_w(uint8_t data);
	void link3_ccw_w(uint8_t data);
	uint8_t link0_ccw_r();
	uint8_t link1_ccw_r();
	uint8_t link2_ccw_r();
	uint8_t link3_ccw_r();

	devcb_write_line m_error_cb;

	///////////////////////////////////////////////////////////////////////////////////
	private:
	int execute_one(int opcode);

	// Process timers.
	u32 cycles_per_microsecond() { return clock() / 1000000; }
	u64 m_last_timer_tick;  		// Cycle with the last timer tick.
	u8 	m_low_priority_usec_to_go;	// Number of usec until the low-priority timer increments.

	// Instruction count stuff.
	int m_icount;
	u64 m_count_before_instruction_step;
	u64 m_bcount;

	// Subsystem flags.
	bool m_boot_from_rom;
	bool m_is_analysed;     // TRUE if ANALYSE pin was asserted during last reset.
	bool m_errorFlag;       // Process error flag, global to the CPU.

	// SRAM registers
	uint32_t m_TPtrLoc0, m_TPtrLoc1;
	uint32_t m_event;
	uint32_t m_WdescIntSaveLoc,
			 m_IptrIntSaveLoc,
			 m_AregIntSaveLoc,
			 m_BregIntSaveLoc,
			 m_CregIntSaveLoc,
			 m_STATUSIntSaveLoc,
			 m_EregIntSaveLoc;

	// write line states
	bool m_analyse_in;
	bool m_reset_in;
	bool m_boot_from_rom_in;

	bool m_cpu_state;
	
	struct {
		transputer_bootstrap::STATE state;
		transputer_bootstrap::COMMAND command;
		uint8_t length_remaining;
		uint8_t code_length;
		uint32_t current_buffer_offset;

		uint32_t command_addr;
		uint32_t command_data;
	} m_bootstrap_status;

	required_device<ram_device> m_internal_sram;
};

#endif