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

class apollo_dn300_mmu_device : public device_t, device_memory_interface
{
public:
	// device type constructor
	apollo_dn300_mmu_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

    uint16_t    pid_priv_r()                { return m_pid_priv_register; }
    uint8_t     status_r()                  { return m_status_register; }

    void        pid_priv_w(uint16_t data);
    void        status_w(uint8_t data);

	uint16_t    disp_1_r(offs_t offset);
	uint16_t    ring_2_r(offs_t offset);
	uint16_t    dsk_r(offs_t offset);
    uint16_t    pft_r(offs_t offset)        { return m_pft[offset / 2]; }
    uint16_t    big_pft_r(offs_t offset)    { return m_big_pft[offset / 2]; }
    uint16_t    ptt_r(offs_t offset)        { return m_ptt[offset / 2]; }
    uint8_t     mem_ctrl_r()                { return m_mcr; }
    uint16_t    mem_status_r()              { return m_msr; }

	void        disp_1_w(offs_t offset, uint16_t data);
	void        ring_2_w(offs_t offset, uint16_t data);
	void        dsk_w(offs_t offset, uint16_t data);
    void        pft_w(offs_t offset, uint16_t data);
    void        big_pft_w(offs_t offset, uint16_t data);
    void        ptt_w(offs_t offset, uint16_t data);
    void        mem_ctrl_w(uint8_t data);
    void        mem_status_w(uint16_t data);

    void        physical_map(address_map &map);
    void        virtual_map(address_map &map);

    uint16_t    mmu_r(offs_t offset);
    void        mmu_w(offs_t offset, uint16_t data);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

    // device_memory_interface overrides
	virtual space_config_vector memory_space_config() const override;

private:
    required_device<m68010_device> m_maincpu;

	address_space_config  m_physical_config;
	address_space_config  m_virtual_config;

	memory_access<24, 16, 0, ENDIANNESS_BIG>::specific m_physical_space;
	memory_access<24, 16, 0, ENDIANNESS_BIG>::specific m_virtual_space;

    uint16_t    m_pid_priv_register;
    uint8_t     m_status_register;

    uint32_t    m_pft[0x800];
    uint16_t    m_big_pft[0x1800];
    uint16_t    m_ptt[0x80000];

    uint8_t     m_mcr;
    uint16_t    m_msr;

    void update_mmu_enabled(bool state);
    void update_display_control(uint16_t data);

    bool m_mmu_enabled = 0;
};

DECLARE_DEVICE_TYPE(APOLLO_DN300_MMU, apollo_dn300_mmu_device)

#endif
