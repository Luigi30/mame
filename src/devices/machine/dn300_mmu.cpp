/***********************************************************
 * Apollo DN300 MMU
 * 
 * Physical Memory Layout
 * 1K PPN
 * 0000-000F	PROM		000000
 * 0010-001F	PFT			004000
 * 0020			MMU			008000
 * 0021			SIO			008400
 * 0022			TIMERS		008800
 * 0023			---
 * 0024			DMA			009000
 * 0025			DISP1		009400
 * 0026			RING		009800
 * 0027			DISKS,CAL	009C00
 * 0028-002A	---
 * 002B			DIAGS		00AC00
 * 002C			FPU CTL		00B000
 * 002D			FPU CMD		00B400
 * 002E			FPU CS		00B800
 * 002F-007F	---
 * 0080-00FF	DISP1 MEM	020000
 * 0100-01FF	---
 * 0200-03FF	PHYS MEM	080000
 * 0400			MD DATA		100000
 * 0401			TRAP PG		100400
 * 0402-0FFF	PHYS MEM	100800
 * 1000-1BFF	PTT(020)	------
 * 1C00-1FFF	PTT			700000
 * 2000-3FFF	---
 * 
 * -------------------------------
 * Virtual Memory Layout
 * 000000-0003FF	TRAP PAGE
 * 000400-003FFF	PROM
 * 007000			FPU CMD
 * 008000-1FFFFF	GLOBAL ADDRESS SPACE
 * 200000-AFFFFF	PRIVATE ADDRESS SPACE
 * 700000-7FFFFF	PTT
 * BC0000-BFFFFF	PRIVATE PROTECTED ADDRESS SPACE
 * E00000-EFFFFF	OS PROC AND DATA
 * F00000-F68000	OS BUFFERS
 * FC0000-FDFFFF	DISP1 MEM
 * FF7000			FPU CTL
 * FF7400			FPU CMD
 * FF7800			FPU CS
 * FF8800			CHK BUFF
 * FF8C00			Z BUFF
 * FF9000			PAR BUFF
 * FF9800			DISP1
 * FF9C00			RING
 * FFA000			DMA
 * FFA800			FLOP,WIN,CAL
 * FFAC00			TIMR
 * FFB000			SIO
 * FFB400			MMU
 * FFB800-FF7FFF	PFT
 */

#include "emu.h"
#include "dn300_mmu.h"
#include "video/dn300.h"

#define LOGGENERAL		0x01
#define LOGMMU_ACCESS	0x02
#define LOGTRANSLATION	0x04
#define LOGBUSERROR		0x08
#define LOGPFTSEARCH	0x10

//#define VERBOSE (LOG_GENERAL|LOG_MMU_ACCESS|LOG_TRANSLATION)
#define VERBOSE (LOGGENERAL|LOGBUSERROR)

#include "logmacro.h"

#define LOG_MMUACCESS(...) 			LOGMASKED(LOGMMU_ACCESS,	__VA_ARGS__)
#define LOG_TRANSLATION(...)		LOGMASKED(LOGTRANSLATION,	__VA_ARGS__)
#define LOG_BUSERROR(...)			LOGMASKED(LOGBUSERROR,		__VA_ARGS__)
#define LOG_PFTSEARCH(...)			LOGMASKED(LOGPFTSEARCH,		__VA_ARGS__)

#ifdef _MSC_VER
#define FUNCNAME __func__
#else
#define FUNCNAME __PRETTY_FUNCTION__
#endif

#define PAGE_SIZE 	( 1024 )
#define PAGENUM(ADDR) ( ADDR / PAGE_SIZE )

DEFINE_DEVICE_TYPE(APOLLO_DN300_MMU, apollo_dn300_mmu_device, "apollo_dn300_mmu", "Apollo DN300 MMU")

apollo_dn300_mmu_device::apollo_dn300_mmu_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, APOLLO_DN300_MMU, tag, owner, clock)
	, device_memory_interface(mconfig, *this)
	, m_maincpu(*this, ":maincpu")
	, m_physical_config("physical", ENDIANNESS_BIG, 16, 24)
	, m_space(*this, finder_base::DUMMY_TAG, -1)
{
}

device_memory_interface::space_config_vector apollo_dn300_mmu_device::memory_space_config() const
{
	return space_config_vector {
		std::make_pair(0, &m_physical_config)
	};
}

void apollo_dn300_mmu_device::device_add_mconfig(machine_config &config)
{

}

void apollo_dn300_mmu_device::device_start()
{
	m_bus_error_timer = timer_alloc(FUNC(apollo_dn300_mmu_device::bus_error), this);
}

void apollo_dn300_mmu_device::device_reset()
{
	m_pid_priv_register = 0;
	m_status_register = 0;
}

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
	 * b0: tied to 0 if 68020, tied to 1 if 68010
	 */

	LOG("%s: D:%X\n", FUNCNAME, data);
	m_status_register = (data & 0x1F);
}

uint16_t apollo_dn300_mmu_device::pft_r(offs_t offset)
{
	// Offset is in words, convert it to bytes.
	offset = offset * 2;

	// One entry every 4 bytes.
	uint16_t index = offset / 4;
	uint16_t result;

	if(BIT(offset, 1))
	{
		// odd = low word
		result = (m_pft[index].value & 0x0000FFFF);
	}
	else
	{
		// even = high word
		result = (m_pft[index].value & 0xFFFF0000) >> 16;
	}

	if(!machine().side_effects_disabled()) LOG_TRANSLATION("%s: O:%06X D:%04X\n", FUNCNAME, offset, result);

	return result;
}

uint16_t apollo_dn300_mmu_device::ptt_r(offs_t offset)
{
	offset = offset*2;
	uint16_t page = offset >> 10;
	
    return m_ptt[page];
}

void apollo_dn300_mmu_device::pft_w(offs_t offset, uint16_t data)
{
    /* page frame table
	 *	
	 * entry format:
	 * b31-b25: address space ID
	 * 	The MMU compares its ASID (the current context) to the PFT entry's ASID.
	 * 	If the global flag is set, this field is ignored.
	 * b24: supervisor domain
	 * 	Page is locked to SUPERVISOR. Privilege violation if accessed when CPU is in USER.
	 * b23: domain (unused)
	 * b22: write access
	 * b21: read access
	 * b20: execute access
	 * b19-b16: excess virtual page number
	 * 	Excess bits from the virtual page number. The XSVPN in the address must match this for a page hit.
	 * b15: end of chain
	 * 	End of a linked list in the PFT. If a match has not been found after two searches, a bus error occurs.
	 * b14: page is modified
	 * 	Set on page write.
	 * b13: page is referenced
	 * 	Set on page read.
	 * b12: page is global
	 * 	Disables the ASID check.
	 * b11-b0: PFT hash thread
	 * 	Contains the physical page number of the next entry whose hashed virtual page number is the same as its own.
	 * 
	 * One entry per physical page of memory.
	 * 
	 * 
	 */

	// Offset is in words, convert it to bytes.
	offset = offset * 2;

	// One entry every 4 bytes.
	uint16_t index = offset / 4;

	uint32_t old = m_pft[index].value;
	//uint16_t endian_swapped_data = ((data & 0xff00) >> 8) | ((data & 0x00ff) << 8);

	// Low word or high word of an entry?
	if(BIT(offset, 1))
	{
		// odd = low word
		m_pft[index].value = (m_pft[index].value & 0xFFFF0000) | data;
	}
	else
	{
		// even = high word
		m_pft[index].value = (m_pft[index].value & 0x0000FFFF) | (((uint32_t)data) << 16);
	}

	uint32_t pfte = m_pft[index].value;

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

	LOG("%s: PC=%06X O:%04X D:%04X | PFT entry %04X %08X->%08X A:%02X|%c|D:%d|%s|P:%01X|%s|L:%03X\n", FUNCNAME, m_maincpu->pc(), offset, data, index, old, pfte,
		pfte >> 25, s, BIT(pfte, 23), wrx, (pfte >> 16) & 15, emug, pfte & 0xFFF);
}

void apollo_dn300_mmu_device::ptt_w(offs_t offset, uint16_t data)
{
	/* Page Translation Table
	 *
	 * Effectively, a cache for the PFT.
	 * Each entry points to one entry in a list of PFT entries.
	 * All linked PFT entries point to a common PTT entry. 
	 * 
	 * Bits 10-21 are an index to the PTT, which points to the PFT, which produces a physical address.
	 * The PTT is assigned to the fixed virtual range $400000-$800000.
	 * Writes are disabled when the PTT update bit is cleared.
	 *
	 * entry format:
	 * b11-b0: physical page number
	 * 
	 * One PPTE every 1024 bytes in table.
	 */

	// Offset is in words, convert it to bytes.
	offset = offset*2;
	uint16_t page = offset >> 10;

	LOG("%s: O:%06X D:%04X | setting PTT entry %03X (offset %06X) to page %04X (%06X-%06X)\n",
		FUNCNAME, offset, data, page, offset, data, data*PAGE_SIZE, (data*PAGE_SIZE)+PAGE_SIZE-1);

    m_ptt[page] = data & 0xFFF;
}

/*****************************************************************************
 * The fun part, if your idea of fun is smashing your balls with a hammer.
 * 
 * AEGIS manual section 10 page 13
 * 
 * When the hardware gets a virtual address to translate, it:
 * 
 * 1. Takes the virtual page number in the virtual address and uses it as an index into the PTT.
 * 
 * 2. Uses the information in the PTTE to locate the first PFTE in the chain.
 * 		If there is no hint in the PTT, the hardware hashes the virtual address to obtain an
 * 		index into the PFT.
 * 
 * 3. Examines the PFTE, searching for a virtual-to-physical address correspondence by checking
 * 		the ASID and XSVPN in the virtual address against the ASID and XSVPN in the PFTE. (On a
 * 		global reference, only the XSVPN must match.) If the two values do not match, the hardware
 * 		searches all PFTEs in the linked list for a match. If the virtual address is not in the list,
 * 		and it has encountered the end-of-chain field twice, the hardware recognizes that no
 * 		virtual-physical page mapping exists for the address and signals a page fault to install
 * 		the correct mapping.
 * 
 * 4. If a match exists, copies the index of the matching PFTE to the PTTE and calculates the
 * 		physical address by combining the offset given in the virtual address with the PPN in the
 * 		PTTE; this operation will optimize performance on the next search.	
 * 
 * Page 1-13 describes the MMU further.
 * For a 26-bit virtual address,
 * 	b00-b09: 10-bit offset
 * 	b10-b21: hashed virtual page number
 *  b25-b22: XSVPN
 * 
 * Theory: for a 24-bit virtual address, we drop it to a 10-bit virtual page. The MMU still has a 4-bit XSVPN field.
 * So for a 24-bit virtual address,
 * 	b00-b09: 10-bit offset
 * 	b10-b19: hashed virtual page number
 *  b20-b23: XSVPN
 * 
 *****************************************************************************/

#define PFTE_IS_END(PFTE) ( BIT(PFTE, 15) )
#define SEARCH_RESULT_PAGE_FAULT		( 0xFFFFFFFF )
#define SEARCH_RESULT_ACCESS_VIOLATION	( 0xFFFFFFFE )

bool apollo_dn300_mmu_device::operation_is_permitted(PFT_ENTRY pfte, uint16_t fc, bool write)
{
	// Short-circuit: if pfte.R is 0, fail.
	if(!pfte.field.read_access) return false;

	// Now check FC bits.
	// At this point R is always 1.
	switch(fc)
	{
		case 1:
		{
			if(pfte.field.supervisor) return false;

			// At this point S is always 0.
			if(!write)
			{
				// User Data Read
				return true;
			}
			else
			{
				// User Data Write
				return pfte.field.write_access;
			}
			break;
		}
		case 2:
		{
			if(pfte.field.supervisor) return false;

			// At this point S is always 0.
			if(!write)
			{
				// User Program Read
				return pfte.field.execute_access;
			}
			else
			{
				// User Program Write
				return false; // Never allowed.
			}
		}
		case 5:
		{
			if(!write)
			{
				// Supervisor Data Read
				return true;
			}
			else
			{
				// Supervisor Data Write
				return pfte.field.write_access;
				
			}
		}
		case 6:	
		{
			if(!write)
			{
				// Supervisor Program Read
				return pfte.field.execute_access;
			}
			else
			{
				// Supervisor Program Write
				return false; // Never allowed.
			}
		}
		default: return false;
	}
}

void apollo_dn300_mmu_device::debug_dump_pfte(PFT_ENTRY pfte, uint16_t current_ppn, V_ADDR vaddr, bool always)
{
	bool end 	= pfte.field.end_of_chain;
	bool global = pfte.field.global;
	auto asid 	= pfte.field.address_space_id;
	auto xsvpn 	= pfte.field.xsvpn;
	auto link 	= pfte.field.link;

	auto vaddr_xsvpn = vaddr.fields.xsvpn;

	if(always)
	{
		LOG("%s: PFTE for page %03X: %08X (MMU ASID %02X XSVPN %01X) (%c END %d ASID %02X XSVPN %01X LINK %03X)\n", FUNCNAME,
			current_ppn, pfte.value, mmu_current_asid(), vaddr_xsvpn, global ? 'G' : '-', end, asid, xsvpn, link);
	}
	else
	{
		LOG_PFTSEARCH("%s: PFTE for page %03X: %08X (MMU ASID %02X XSVPN %01X) (%c END %d ASID %02X XSVPN %01X LINK %03X)\n", FUNCNAME,
			current_ppn, pfte.value, mmu_current_asid(), vaddr_xsvpn, global ? 'G' : '-', end, asid, xsvpn, link);
	}

}

uint32_t apollo_dn300_mmu_device::pft_search(PFT_ENTRY chain_start, V_ADDR vaddr, bool write)
{
	uint16_t head_ppn = chain_start.field.link;

	//if(m_bus_error) LOG_BUSERROR("%s: searching chain %03X for vaddr %06X\n", FUNCNAME, head_ppn, vaddr.address);

	if(!machine().side_effects_disabled()) LOG_PFTSEARCH("%s: searching chain %03X for vaddr %06X\n", FUNCNAME, head_ppn, vaddr.address);

	int end_count = 0;	// Times END bit has been encountered.

	unsigned int current_link = chain_start.field.link;
	do
	{
		// Latch link register with next PPN.
		uint16_t current_ppn = current_link;

		if(m_bus_error) LOG_BUSERROR("%s: PTT %03X: PPN %03X\n", FUNCNAME, current_link, current_ppn);
		if(!machine().side_effects_disabled()) LOG_PFTSEARCH("%s: PTT %03X: PPN %03X\n", FUNCNAME, current_link, current_ppn);

		// Use new PPN to re-index PFT.
		PFT_ENTRY pfte = m_pft[current_ppn];

		if(!machine().side_effects_disabled()) debug_dump_pfte(pfte, current_ppn, vaddr, false);
		if(m_bus_error) debug_dump_pfte(pfte, current_ppn, vaddr, true);

		// Parity check.

		// EOL bit set?
		if(pfte.field.end_of_chain) 
		{ 
			end_count++;
			if(m_bus_error) LOG_BUSERROR("%s: end of chain\n", FUNCNAME);
			if(!machine().side_effects_disabled()) LOG_PFTSEARCH("%s: end of chain\n", FUNCNAME);
		}
		
		// Two EOLs?
		if(end_count == 2)
		{
			if(m_bus_error) LOG_BUSERROR("%s: hit end of chain twice, page faulting\n", FUNCNAME);
			if(!machine().side_effects_disabled()) debug_dump_pfte(m_pft[current_ppn], current_ppn, vaddr, true);
			return SEARCH_RESULT_PAGE_FAULT;
		}

		bool global = m_pft[current_ppn].field.global;
		auto asid 	= m_pft[current_ppn].field.address_space_id;
		auto xsvpn 	= m_pft[current_ppn].field.xsvpn;
		if(!machine().side_effects_disabled())
			debug_dump_pfte(pfte, current_ppn, vaddr, false);

		bool xsvpns_match = (vaddr.fields.xsvpn == xsvpn);
		bool asids_match = (mmu_current_asid() == asid) || global;
		bool permissions_ok = operation_is_permitted(m_pft[current_ppn], m_maincpu->get_fc(), write);

		if(xsvpns_match && asids_match)
		{
			if(!permissions_ok)
			{
				if(!machine().side_effects_disabled()) debug_dump_pfte(pfte, current_ppn, vaddr, true);
				return SEARCH_RESULT_ACCESS_VIOLATION;
			}

			if(write == 0) 
				m_pft[current_ppn].field.used = true;
			else if(write == 1)
				m_pft[current_ppn].field.modified = true;

			if(m_bus_error) LOG_BUSERROR("%s: matched to %06X\n", FUNCNAME, (current_ppn << 10) | vaddr.fields.offset);
			return (current_ppn << 10) | vaddr.fields.offset;
		}
		else
		{
			// Miss. Next.
			current_link = pfte.field.link;
			continue;
		}
	} while(true);
}

bool apollo_dn300_mmu_device::memory_translate(int spacenum, int intention, offs_t &address)
{
	// We have a virtual address. We need to produce a physical address.

	// Flowchart: "Use HVPN to index PTT, which contains a PPN."

	bool write = intention == TRANSLATE_WRITE;

	if(!machine().side_effects_disabled()) LOG_TRANSLATION("%s: -- begin\n", FUNCNAME);
	// P_ADDR paddr;
	V_ADDR vaddr;
	
	vaddr.address = address;
	
	uint16_t vxsvpn = vaddr.fields.xsvpn;
	uint16_t vpage = vaddr.fields.page;
	uint16_t voffset = vaddr.fields.offset;

	uint16_t ptt_entry = m_ptt[vpage] & 0xFFF;
	uint16_t current_page = ptt_entry;

	if(m_bus_error)
		LOG_BUSERROR("%s: translating vaddr %06X (%01X:%02X:%03X) -> page %03X -> PTT %03X\n", FUNCNAME,
			vaddr.address, vxsvpn, vpage, voffset, vpage, current_page);
	else if(!machine().side_effects_disabled())
		LOG_TRANSLATION("%s: translating vaddr %06X (%01X:%02X:%03X) -> page %03X -> PTT %03X\n", FUNCNAME,
			vaddr.address, vxsvpn, vpage, voffset, vpage, current_page);


	// Flowchart: Use PPN from PTT to index PFT.
	{
		bool global = m_pft[current_page].field.global;
		auto asid 	= m_pft[current_page].field.address_space_id;
		auto xsvpn 	= m_pft[current_page].field.xsvpn;
		if(!machine().side_effects_disabled()) debug_dump_pfte(m_pft[current_page], current_page, vaddr, false);

		bool xsvpns_match = (vaddr.fields.xsvpn == xsvpn);
		bool asids_match = (mmu_current_asid() == asid) || global;
		bool permissions_ok = operation_is_permitted(m_pft[current_page], m_maincpu->get_fc(), write);

		if(xsvpns_match && asids_match)
		{
			if(!permissions_ok)
			{
				if(!machine().side_effects_disabled()) debug_dump_pfte(m_pft[current_page], current_page, vaddr, true);
				access_violation(vaddr.address, write, m_current_mask);
				return false;
			}

			if(intention == TRANSLATE_READ) 
				m_pft[current_page].field.used = true;
			else if(intention == TRANSLATE_WRITE)
				m_pft[current_page].field.modified = true;

			address = (current_page << 10) | vaddr.fields.offset;
		}
		else
		{
			char r = m_pft[current_page].field.read_access ? 'R' : '-';
			char w = m_pft[current_page].field.write_access ? 'W' : '-';
			char x = m_pft[current_page].field.execute_access ? 'X' : '-';

			// XSVPN is virtual addr field vs PFTE field
			// ASIO is MMU vs PFTE field
			if(m_bus_error) LOG_BUSERROR("%s: PFT miss for vaddr %06X (XSVPN %01X|%01X, ASID %02X|%02X, %c%c%c), initiate PFT search.\n", FUNCNAME, vaddr.address, vxsvpn, xsvpn, mmu_current_asid(), asid, r, w, x);
			else if(!machine().side_effects_disabled()) LOG_TRANSLATION("%s: PFT miss for vaddr %06X (XSVPN %01X|%01X, ASID %02X|%02X, %c%c%c), initiate PFT search.\n", FUNCNAME, vaddr.address, vxsvpn, xsvpn, mmu_current_asid(), asid, r, w, x);
			uint32_t result = pft_search(m_pft[current_page], vaddr, write);

			if(result == SEARCH_RESULT_PAGE_FAULT)
			{
				// Search failed.
				if(!machine().side_effects_disabled()) debug_dump_pfte(m_pft[current_page], current_page, vaddr, true);
				page_fault(vaddr.address, write, m_current_mask);
				return false;

			}
			else if(result == SEARCH_RESULT_ACCESS_VIOLATION)
			{
				access_violation(vaddr.address, write, m_current_mask);
				return false;
			}
			else
			{
				// Search successful.
				if(!machine().side_effects_disabled()) LOG_TRANSLATION("%s: %06X -> %06X\n", FUNCNAME, vaddr.address, result);
				address = result;
			}
		}
	}

	return true;
}

void apollo_dn300_mmu_device::page_fault(offs_t address, bool write, uint16_t mask)
{
	if(!machine().side_effects_disabled()) LOG_BUSERROR("%s: pc=%06X page fault at %06X, write %d\n", FUNCNAME, m_maincpu->pc(), address, write);

	// Set the access violation bit.
	m_status_register |= 0x40;
	// Trigger bus error.
	set_bus_error(address, write, m_current_mask);
}

void apollo_dn300_mmu_device::access_violation(offs_t address, bool write, uint16_t mask)
{
	if(!machine().side_effects_disabled()) LOG_BUSERROR("%s: pc=%06X access violation at %06X, write %d\n", FUNCNAME, m_maincpu->pc(), address, write);

	// Set the access violation bit.
	m_status_register |= 0x80;
	// Trigger bus error.
	set_bus_error(address, write, m_current_mask);
}

uint16_t apollo_dn300_mmu_device::translated_read(offs_t offset, uint16_t mem_mask)
{
	offs_t vaddr = offset*2;
	offs_t paddr = offset*2;
	m_current_mask = mem_mask;
	memory_translate(AS_PROGRAM, TRANSLATE_READ, paddr);

	if(!machine().side_effects_disabled())
		LOG_MMUACCESS("%s: MMU READ,  translating vaddr %06X to paddr %06X, mask %04X\n", FUNCNAME, vaddr, paddr, mem_mask);

	uint16_t result;

	switch(mem_mask)
	{
		case 0x00FF: result = m_space->read_byte(paddr+1); break;
		case 0xFF00: result = m_space->read_byte(paddr) << 8; break;
		case 0xFFFF: result = m_space->read_word(paddr, mem_mask); break;
		default: fatalerror("%s: unhandled mask %02X", FUNCNAME, mem_mask);
	}

	return result;
}

void apollo_dn300_mmu_device::translated_write(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	offs_t vaddr = offset*2;
	offs_t paddr = offset*2;
	m_current_mask = mem_mask;
	memory_translate(AS_PROGRAM, TRANSLATE_WRITE, paddr);

	if(!machine().side_effects_disabled())
		LOG_MMUACCESS("%s: MMU WRITE, translating vaddr %06X to paddr %06X, data %04X, mask %04X\n", FUNCNAME, vaddr, paddr, data, mem_mask);

	switch(mem_mask)
	{
		case 0x00FF: m_space->write_byte(paddr+1, data>>8); break;
		case 0xFF00: m_space->write_byte(paddr, data); break;
		case 0xFFFF: m_space->write_word(paddr, data, mem_mask); break;
		default: break;
	}
}

void apollo_dn300_mmu_device::set_bus_error(uint32_t address, bool write, uint16_t mem_mask)
{
	// 1 for read, 0 for write. Thanks, Motorola.
	bool rw = !write;

	if(machine().side_effects_disabled())
	{
		return;
	}
	if(m_bus_error)
		return;

	LOG_BUSERROR("%s: addr %06X, write? %d, mem_mask %04X\n", FUNCNAME, address, write, mem_mask);

	if(!ACCESSING_BITS_8_15)
		address++;

	m_bus_error = true;
	m_maincpu->set_buserror_details(address, rw, m_maincpu->get_fc());
	m_maincpu->set_input_line(M68K_LINE_BUSERROR, ASSERT_LINE);
	m_maincpu->set_input_line(M68K_LINE_BUSERROR, CLEAR_LINE);
	m_bus_error_timer->adjust(m_maincpu->cycles_to_attotime(16)); // let rmw cycles complete
}

TIMER_CALLBACK_MEMBER(apollo_dn300_mmu_device::bus_error)
{
	m_bus_error = false;
}