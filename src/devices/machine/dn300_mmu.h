// license:BSD-3-Clause
// copyright-holders:Katherine Rohl

// Apollo DN300 MMU.
// All memory access goes through the MMU.

#ifndef MAME_DN300_MMU
#define MAME_DN300_MMU

#pragma once

#include "cpu/m68000/m68000.h"
#include "machine/hd63450.h"
#include "machine/mc68681.h"
#include "machine/6850acia.h"
#include "machine/upd765.h"
#include "machine/6840ptm.h"
#include "machine/mc146818.h"
#include "machine/ram.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> apollo_dn300_mmu_device

class apollo_dn300_mmu_device : public device_t, public device_memory_interface
{
private:
private:
    typedef union
    {
        uint32_t value;
        struct
        {
            unsigned int link				: 12;   // b0-b11 Next PFTE in chain
            unsigned int global				: 1;    // b12
            unsigned int used				: 1;    // b13
            unsigned int modified			: 1;    // b14
            unsigned int end_of_chain		: 1;    // b15
            unsigned int xsvpn 				: 4;    // b16-b19 Excess Virtual Page Number
            unsigned int execute_access 	: 1;    // b20 Access Rights
            unsigned int read_access 		: 1;    // b21 Access Rights
            unsigned int write_access 		: 1;    // b22 Access Rights
            unsigned int domain 			: 1;    // b23 Access Rights
            unsigned int supervisor 		: 1;    // b24 Access Rights
            unsigned int address_space_id   : 7;    // b25-b31 ASID 0 is global
        } field;
    } PFT_ENTRY;

    typedef struct
    {
        unsigned int offset : 10;   // 1024 bytes per page
        unsigned int page   : 10;   // 1024 pages mapped
        unsigned int xsvpn  : 4;    // one of 16 XSVPNs
    } V_ADDR_FORMAT;

    typedef struct
    {
        unsigned int offset : 10;   // 1024 bytes per page
        unsigned int page   : 12;   // 4095 physical pages
    } P_ADDR_FORMAT;

    typedef union
    {
        uint32_t address;
        V_ADDR_FORMAT fields;
    } V_ADDR;

    typedef union
    {
        uint32_t address;
        P_ADDR_FORMAT fields;
    } P_ADDR;

public:
	// device type constructor
	apollo_dn300_mmu_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	template <typename T> void set_space(T &&tag, int spacenum) { m_space.set_tag(std::forward<T>(tag), spacenum); }

    uint16_t    pid_priv_r()                { return m_pid_priv_register; }
    uint8_t     status_r()                  { return m_status_register | 0x01; }

    void        pid_priv_w(uint16_t data);
    void        status_w(uint8_t data);

	uint16_t    disp_1_r(offs_t offset);
	uint16_t    ring_2_r(offs_t offset);
	uint16_t    dsk_r(offs_t offset);
    uint16_t    pft_r(offs_t offset);        //{ return m_pft[offset / 2].value; }
    uint16_t    ptt_r(offs_t offset);        //{ return m_ptt[offset / 1024]; }
    uint8_t     mem_ctrl_r()                { return m_mcr; }
    uint16_t    mem_status_r()              { return m_msr; }

	void        disp_1_w(offs_t offset, uint16_t data);
	void        ring_2_w(offs_t offset, uint16_t data);
	void        dsk_w(offs_t offset, uint16_t data);
    void        pft_w(offs_t offset, uint16_t data);
    void        ptt_w(offs_t offset, uint16_t data);
    void        mem_ctrl_w(uint8_t data);
    void        mem_status_w(uint16_t data);

    void        physical_map(address_map &map);

    uint8_t     mmu_hibyte_r(offs_t offset);
    void        mmu_hibyte_w(offs_t offset, uint8_t data);
    uint8_t     mmu_lobyte_r(offs_t offset);
    void        mmu_lobyte_w(offs_t offset, uint8_t data);

    bool        mmu_is_enabled()    { return BIT(m_pid_priv_register, 0); }
    bool        ptt_is_enabled()    { return BIT(m_pid_priv_register, 1); }
    bool        mmu_domain()        { return BIT(m_pid_priv_register, 2); }
    uint8_t     mmu_current_asid()  { return (m_pid_priv_register >> 8) & 0x7F; }

    uint16_t translated_read(offs_t offset, uint16_t mem_mask);
    void translated_write(offs_t offset, uint16_t data, uint16_t mem_mask);

    // uint8_t translated_read(offs_t offset);
    // void translated_write(offs_t offset, uint8_t data);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
    virtual void device_add_mconfig(machine_config &config) override;

    // device_memory_interface overrides
	virtual space_config_vector memory_space_config() const override;
    virtual bool memory_translate(int spacenum, int intention, offs_t &address) override;

    bool operation_is_permitted(PFT_ENTRY pfte, uint16_t fc, bool write);

    emu_timer* m_bus_error_timer = nullptr;
    TIMER_CALLBACK_MEMBER(bus_error);
    void set_bus_error(uint32_t address, bool write, uint16_t mem_mask);
	bool m_bus_error = false;

    uint16_t m_current_mask;

    required_device<m68010_device> m_maincpu;

	address_space_config    m_physical_config;
    required_address_space  m_space;

    uint16_t    m_pid_priv_register;
    uint8_t     m_status_register;

    PFT_ENTRY   m_pft[0x4000];  // One per physical page.
    uint16_t    m_ptt[0x400];   // 1024 entries.

    uint8_t     m_mcr;
    uint16_t    m_msr;

    void update_mmu_enabled(bool state);
    void update_display_control(uint16_t data);

    uint32_t pft_search(PFT_ENTRY chain_start, V_ADDR vaddr, bool rw);

    void debug_dump_pfte(PFT_ENTRY pfte, uint16_t current_ppn);

    void page_fault(offs_t address, bool write, uint16_t mask);
    void access_violation(offs_t address, bool write, uint16_t mask);
};

DECLARE_DEVICE_TYPE(APOLLO_DN300_MMU, apollo_dn300_mmu_device)

#endif
