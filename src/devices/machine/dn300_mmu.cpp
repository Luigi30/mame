#include "emu.h"
#include "dn300_mmu.h"
#include "video/dn300.h"

#define VERBOSE 1
#include "logmacro.h"

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define PAGE_SIZE 	( 1024 )

DEFINE_DEVICE_TYPE(APOLLO_DN300_MMU, apollo_dn300_mmu_device, "apollo_dn300_mmu", "Apollo DN300 MMU")

apollo_dn300_mmu_device::apollo_dn300_mmu_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, APOLLO_DN300_MMU, tag, owner, clock)
	, device_memory_interface(mconfig, *this)
	, m_maincpu(*this, ":maincpu")
	, m_physical_config("physical", 	ENDIANNESS_BIG, 16, 24, 0, address_map_constructor(FUNC(apollo_dn300_mmu_device::physical_map), this))
	, m_virtual_config("virtual", 		ENDIANNESS_BIG, 16, 24, 0, address_map_constructor(FUNC(apollo_dn300_mmu_device::virtual_map), this))
{
}

device_memory_interface::space_config_vector apollo_dn300_mmu_device::memory_space_config() const
{
	return space_config_vector {
		std::make_pair(0, &m_physical_config),
		std::make_pair(1, &m_virtual_config)
	};
}

void apollo_dn300_mmu_device::device_start()
{
	// address_space &space = m_maincpu->space(AS_PROGRAM);
	// space.unmap_readwrite(0x000000, 0xffffff);
	// space.install_readwrite_handler(0x000000, 0xffffff, 
	// 	read16sm_delegate(*this, FUNC(apollo_dn300_mmu_device::mmu_r)), write16sm_delegate(*this, FUNC(apollo_dn300_mmu_device::mmu_w)));

	space(0).specific(m_physical_space);
	space(1).specific(m_virtual_space);
}

void apollo_dn300_mmu_device::device_reset()
{
	update_mmu_enabled(false);
}

void apollo_dn300_mmu_device::virtual_map(address_map &map)
{
	// Set up a virtual address map based on the contents of the page tables.
	map.unmap_value_high();
	LOG("%s: Switching to virtual address map...\n", FUNCNAME);
	map(0x000000, 0xFFFFFF).rw(FUNC(apollo_dn300_mmu_device::mmu_r), FUNC(apollo_dn300_mmu_device::mmu_w));
}

void apollo_dn300_mmu_device::physical_map(address_map &map)
{
	LOG("%s: Switching to physical address map...\n", FUNCNAME);

	// Physical addresses.
	map.unmap_value_high();
	map(0x000000, 0x003FFF).rom().region(":prom", 0);
	map(0x004000, 0x004FFF).rw(FUNC(apollo_dn300_mmu_device::pft_r), FUNC(apollo_dn300_mmu_device::pft_w));
	map(0x005000, 0x007FFF).rw(FUNC(apollo_dn300_mmu_device::big_pft_r), FUNC(apollo_dn300_mmu_device::big_pft_w));
	map(0x008000, 0x008001).rw(FUNC(apollo_dn300_mmu_device::pid_priv_r), FUNC(apollo_dn300_mmu_device::pid_priv_w));
	map(0x008002, 0x008003).rw(FUNC(apollo_dn300_mmu_device::status_r), FUNC(apollo_dn300_mmu_device::status_w));
	map(0x008004, 0x008005).rw(FUNC(apollo_dn300_mmu_device::mem_ctrl_r), FUNC(apollo_dn300_mmu_device::mem_ctrl_w)).umask16(0x00FF);
	map(0x008006, 0x008007).rw(FUNC(apollo_dn300_mmu_device::mem_status_r), FUNC(apollo_dn300_mmu_device::mem_status_w));
	map(0x008400, 0x00841F).rw(":duart", FUNC(scn2681_device::read), FUNC(scn2681_device::write)).umask16(0x00FF);	// Serial I/O Interface
	map(0x008420, 0x008423).rw(":acia", FUNC(acia6850_device::read), FUNC(acia6850_device::write)).umask16(0x00FF);	// Display Keyboard Interface
	map(0x008800, 0x008803).rw(":ptm", FUNC(ptm6840_device::read), FUNC(ptm6840_device::write)); 					// TIMERS
	map(0x009000, 0x0090FF).rw(":dmac", FUNC(hd63450_device::read), FUNC(hd63450_device::write));					// DMA CTL
	map(0x009400, 0x00941F).rw(":display", FUNC(dn300_display_device::regs_r), FUNC(dn300_display_device::regs_w)); // DISP 1
	map(0x009800, 0x009803).rw(FUNC(apollo_dn300_mmu_device::ring_2_r), FUNC(apollo_dn300_mmu_device::ring_2_w)); 	// RING 2
	// Winchester controller at 0x9C00-0x9C0F
	map(0x009C00, 0x009C0F).rw(FUNC(apollo_dn300_mmu_device::dsk_r), FUNC(apollo_dn300_mmu_device::dsk_w));
	map(0x009C10, 0x009C15).m(":fdc", FUNC(upd765a_device::map)).umask16(0x00FF);									// FLP
	map(0x009C20, 0x009C25).rw(":rtc", FUNC(mc146818_device::read), FUNC(mc146818_device::write)).umask16(0x00FF);	// CAL
	map(0x020000, 0x03FFFF).rw(":display", FUNC(dn300_display_device::vram_r), FUNC(dn300_display_device::vram_w));
	map(0x100000, 0x4FFFFF).ram().share(":phys_mem");
	map(0x700000, 0x7FFFFF).rw(FUNC(apollo_dn300_mmu_device::ptt_r), FUNC(apollo_dn300_mmu_device::ptt_w)); 		// PTT
}

//

void apollo_dn300_mmu_device::update_mmu_enabled(bool state)
{
	LOG("%s MMU enabled bit now %d\n", FUNCNAME, state);

	if(m_mmu_enabled && !state)
	{
		// 1 to 0 transition
		m_maincpu->set_addrmap(AS_PROGRAM, &apollo_dn300_mmu_device::physical_map);
	}
	else if(!m_mmu_enabled && state)
	{
		// 0 to 1 transition
		address_space &space = m_maincpu->space(AS_PROGRAM);
		space.unmap_readwrite(0x000000, 0xffffff);
		space.install_readwrite_handler(0x000000, 0xffffff, 
			read16sm_delegate(*this, FUNC(apollo_dn300_mmu_device::mmu_r)), write16sm_delegate(*this, FUNC(apollo_dn300_mmu_device::mmu_w)));
	}
}

//

uint16_t apollo_dn300_mmu_device::ring_2_r(offs_t offset)
{
	LOG("%s O:%X\n", FUNCNAME, offset);
	return 0;
}

uint16_t apollo_dn300_mmu_device::dsk_r(offs_t offset)
{
	LOG("%s O:%X\n", FUNCNAME, offset);
	return 0;
}

void apollo_dn300_mmu_device::ring_2_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_mmu_device::dsk_w(offs_t offset, uint16_t data)
{
	LOG("%s O:%X D:%04X\n", FUNCNAME, offset, data);
}

void apollo_dn300_mmu_device::mem_ctrl_w(uint8_t data)
{
	/*
	 * b7-b4: LEDs (top to bottom)
	 * b3: parity during DMA cycle
	 * b2: force left byte parity
	 * b1: force right byte parity
	 * b0: disable parity err traps
	 */

	LOG("%s: D:%X\n", FUNCNAME, data);
	LOG("%s: LEDs now %d%d%d%d\n", FUNCNAME, BIT(data,7), BIT(data,6), BIT(data,5), BIT(data,4));
	m_mcr = data;
}

void apollo_dn300_mmu_device::mem_status_w(uint16_t data)
{
	/*
	 * b15-b4: failing PPN
	 * b3: reserved
	 * b2: left byte parity error
	 * b1: right byte parity error
	 * b0: parity error traps enabled
	 */

	LOG("%s: D:%X\n", FUNCNAME, data);
	m_msr = data;
}

void apollo_dn300_mmu_device::pid_priv_w(uint16_t data)
{
	/*
	 * b8-b14: ASID
	 * b2: domain
	 * b1: enable PTT access
	 * b0: enable MMU
	 */

	LOG("%s: D:%X\n", FUNCNAME, data);

	update_mmu_enabled(BIT(data, 0));

    m_pid_priv_register = data;

    // When the MMU is enabled, all reads/writes from the CPU go through the page tables.
    // Otherwise, they are physical accesses.
}

void apollo_dn300_mmu_device::status_w(uint8_t data)
{
    /*
	 * b7: access violation
	 * b6: page fault
	 * b5: bus timeout
	 * b4: normal mode
	 * b3: interrupt pending
	 * b2: 4K PFT
	 * b1: PTT access enabled
	 * b0: MMU enabled
	 */

	LOG("%s: D:%X\n", FUNCNAME, data);
	m_status_register = data;
}

void apollo_dn300_mmu_device::pft_w(offs_t offset, uint16_t data)
{
    /* page frame table
	 *	
	 * entry format:
	 * b31-b25: address space ID
	 * b24: supervisor domain
	 * b23: domain
	 * b22: write access
	 * b21: read access
	 * b20: execute access
	 * b19-b16: excess virtual page number
	 * b15: end of chain
	 * b14: page is modified
	 * b13: page is referenced
	 * b12: page is global
	 * b11-b0: PFT hash thread
	 * 
	 * One entry per physical page of memory.
	 */

    //LOG("%s: O:%06X D:%04X\n", FUNCNAME, offset, data);
	if(offset % 2)
	{
		// odd = low word
		m_pft[offset / 2] = (m_pft[offset / 2] & 0xFFFF0000) | data;
	}
	else
	{
		// even = high word
		m_pft[offset / 2] = (m_pft[offset / 2] & 0x0000FFFF) | (((uint32_t)data) << 16);
	}

	uint32_t pfte = m_pft[offset/2];

	char s = BIT(pfte, 23) ? 'S' : '-';

	char wrx[4] = { 0,0,0,0 };
	wrx[0] = BIT(pfte, 22) ? 'W' : '-';
	wrx[1] = BIT(pfte, 21) ? 'R' : '-';
	wrx[2] = BIT(pfte, 20) ? 'X' : '-';

	char emug[5] = {0,0,0,0,0};
	emug[0] = BIT(pfte, 15) ? 'E' : '-';
	emug[1] = BIT(pfte, 14) ? 'M' : '-';
	emug[2] = BIT(pfte, 13) ? 'U' : '-';
	emug[3] = BIT(pfte, 12) ? 'G' : '-';

	//LOG("%s: writing PFT entry for page %06X. now %08X\n", FUNCNAME, offset, m_pft[offset/2]);
	LOG("%s: entry %04X A:%02X|%c|D:%d|%s|P:%01X|%s|L:%03X\n", FUNCNAME, offset/2,
		pfte >> 25, s, BIT(pfte, 23), wrx, (pfte >> 16) & 15, emug, pfte & 0x7F);
}

void apollo_dn300_mmu_device::big_pft_w(offs_t offset, uint16_t data)
{
    /* page frame table
	 *	
	 * entry format:
	 * b31-b25: address space ID
	 * b24: supervisor domain
	 * b23: domain
	 * b22: write access
	 * b21: read access
	 * b20: execute access
	 * b19-b16: excess virtual page number
	 * b15: end of chain
	 * b14: page is modified
	 * b13: page is referenced
	 * b12: page is global
	 * b11-b0: PFT hash thread
	 * 
	 * One entry per physical page of memory.
	 */

    //LOG("%s: O:%06X D:%04X\n", FUNCNAME, offset, data);
	if(offset % 2)
	{
		// odd = low word
		m_big_pft[offset / 2] = (m_pft[offset / 2] & 0xFFFF0000) | data;
	}
	else
	{
		// even = high word
		m_big_pft[offset / 2] = (m_pft[offset / 2] & 0x0000FFFF) | (((uint32_t)data) << 16);
	}

	uint32_t pfte = m_pft[offset/2];

	char s = BIT(pfte, 23) ? 'S' : '-';

	char wrx[4] = { 0,0,0,0 };
	wrx[0] = BIT(pfte, 22) ? 'W' : '-';
	wrx[1] = BIT(pfte, 21) ? 'R' : '-';
	wrx[2] = BIT(pfte, 20) ? 'X' : '-';

	char emug[5] = { 0,0,0,0,0 };
	emug[0] = BIT(pfte, 15) ? 'E' : '-';
	emug[1] = BIT(pfte, 14) ? 'M' : '-';
	emug[2] = BIT(pfte, 13) ? 'U' : '-';
	emug[3] = BIT(pfte, 12) ? 'G' : '-';

	//LOG("%s: writing PFT entry for page %06X. now %08X\n", FUNCNAME, offset, m_pft[offset/2]);
	LOG("%s: entry %04X A:%02X|%c|D:%d|%s|P:%01X|%s|L:%03X\n", FUNCNAME, offset/2,
		pfte >> 25, s, BIT(pfte, 23), wrx, (pfte >> 16) & 15, emug, pfte & 0x7F);
}

void apollo_dn300_mmu_device::ptt_w(offs_t offset, uint16_t data)
{
	/* Page Translation Table
	 *
	 * entry format:
	 * b11-b0: physical page number
	 * 
	 * One PPTE every 1024 bytes in table.
	 * 
	 * I think pages are 1024 bytes. Confirm?
	 */

	LOG("%s: setting entry at offset %06X to page %04X (%06X-%06X)\n",
		FUNCNAME, offset*2, data, data*PAGE_SIZE, (data*PAGE_SIZE)+PAGE_SIZE-1);

    m_ptt[offset/512] = data;
}

/*****************************************************************************
 * The fun part, if your idea of fun is smashing your balls with a hammer.
 * 
 *****************************************************************************/
#define PAGENUM(ADDR) ( ADDR / 1024 )

uint16_t apollo_dn300_mmu_device::mmu_r(offs_t offset)
{
	// The MMU is enabled and the CPU is reading something somewhere.
	uint16_t page = PAGENUM(offset);
	uint16_t ptt_entry = m_ptt[page];
	uint32_t base = (ptt_entry * 1024) + (offset % 1024);

	LOG("%s: ptt entry %04X, virtual offset %06X -> physical offset %06X\n", FUNCNAME, ptt_entry, offset, base);

	return m_physical_space.read_word(base);
}

void apollo_dn300_mmu_device::mmu_w(offs_t offset, uint16_t data)
{
	// The MMU is enabled and the CPU is writing something somewhere.
}
