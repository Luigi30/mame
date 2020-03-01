// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    TMS34010 Video Card by Katherine Rohl - Experimental (don't commit to MAME)

**********************************************************************/

#include "emu.h"
#include "screen.h"
#include "tms_video.h"

#include "cpu/tms34010/tms34010.h"
#include "video/tlc34076.h"

#define SCREEN_TAG		"screen"

#define MASTER_CLOCK_40MHz      (XTAL(40'000'000))

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> tms_video_device

class tms_video_device : public device_t, public device_rc2014_card_interface
{
public:
	// construction/destruction
	tms_video_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	// device-specific overrides
	virtual ioport_constructor device_input_ports() const override;
	virtual void device_start() override;
	virtual void device_reset() override;

	virtual void device_add_mconfig(machine_config &config) override;

    virtual const tiny_rom_entry *device_rom_region() const override;

private:
	// object finder
    required_device<tms34010_device> m_maincpu;
    required_device<tlc34076_device> m_tlc34076;

	required_shared_ptr<uint16_t> m_vram;

	rc2014_slot_device *m_slot;

    void tms_map(address_map &map);
    
    TMS340X0_SCANLINE_RGB32_CB_MEMBER(scanline_update);
    TMS340X0_TO_SHIFTREG_CB_MEMBER(to_shiftreg);
	TMS340X0_FROM_SHIFTREG_CB_MEMBER(from_shiftreg);
};

DEFINE_DEVICE_TYPE_PRIVATE(RC2014_TMS_VIDEO, device_rc2014_card_interface, tms_video_device, "tms_video", "TMS34010 Video Card")


//**************************************************************************
//  CONFIGURATION SETTINGS
//**************************************************************************

    /* Jumpers that enable/disable address lines. */

	// TODO: Find a better way to describe these.
static INPUT_PORTS_START(tms_video)

INPUT_PORTS_END


//**************************************************************************
//  DEVICE DEFINITION
//**************************************************************************

//-------------------------------------------------
//  poly_16k_ram_device - constructor
//-------------------------------------------------

tms_video_device::tms_video_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, RC2014_TMS_VIDEO, tag, owner, clock)
	, device_rc2014_card_interface(mconfig, *this)
	, m_maincpu(*this, "maincpu")
    , m_tlc34076(*this, "tms34076")
    , m_vram(*this, "vram")
	, m_slot(nullptr)
{
}

/*************************************
 *
 *  Video update
 *
 *************************************/

TMS340X0_SCANLINE_RGB32_CB_MEMBER(tms_video_device::scanline_update)
{
	// Nominally ignore all the GSP row/column addressing junk.
	// Pretend we have a DAC that just fetches from VRAM automatically.

	uint16_t *vram = m_vram;
	uint32_t *dest = &bitmap.pix32(scanline);
	uint16_t stride = params->hsblnk - params->heblnk;

	for(int i=0; i<stride; i++)
	{
		// Convert two RRRGGGBB pixels to 32-bit values
		uint8_t px = (vram[(scanline * stride) + i] & 0xFF00) >> 8;

		dest[i] = vram[(scanline * stride) + i];
		dest[i+1] = vram[(scanline * stride) + i];
	}
}

//-------------------------------------------------
//  device_input_ports - input port construction
//-------------------------------------------------

ioport_constructor tms_video_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(tms_video);
}

void tms_video_device::tms_map(address_map &map)
{
	map(0x00c00000, 0x00c000ff).rw(m_tlc34076, FUNC(tlc34076_device::read), FUNC(tlc34076_device::write)).umask16(0x00ff);
    map(0x00000000, 0x000fffff).rom().region("maincpu", 0);
	map(0x20000000, 0x201fffff).ram().share("vram");
	map(0xffff0000, 0xffffffff).ram();
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void tms_video_device::device_start()
{

}

void tms_video_device::device_reset()
{
	// m_slot = dynamic_cast<rc2014_slot_device *>(owner());
	// cpu_device &cpu = m_slot->cpu();
	// address_space &space = cpu.space(AS_IO);

	// space.install_readwrite_handler(io_base+0x18, io_base+0x18, read8smo_delegate(*m_vdp, FUNC(tms9918a_device::vram_read)), write8smo_delegate(*m_vdp, FUNC(tms9918a_device::vram_write)));
	// space.install_readwrite_handler(io_base+0x19, io_base+0x19, read8smo_delegate(*m_vdp, FUNC(tms9918a_device::register_read)), write8smo_delegate(*m_vdp, FUNC(tms9918a_device::register_write)));

}

TMS340X0_TO_SHIFTREG_CB_MEMBER(tms_video_device::to_shiftreg)
{
	logerror("%s:to_shiftreg(%08X)\n", machine().describe_context(), address);
}


TMS340X0_FROM_SHIFTREG_CB_MEMBER(tms_video_device::from_shiftreg)
{
	logerror("%s:from_shiftreg(%08X)\n", machine().describe_context(), address);
}

void tms_video_device::device_add_mconfig(machine_config &config)
{
	m_slot = dynamic_cast<rc2014_slot_device *>(owner());

	TMS34010(config, m_maincpu, MASTER_CLOCK_40MHz);
	m_maincpu->set_addrmap(AS_PROGRAM, &tms_video_device::tms_map);
	m_maincpu->set_halt_on_reset(false);
	m_maincpu->set_pixel_clock(MASTER_CLOCK_40MHz/6);
	m_maincpu->set_pixels_per_clock(1);
	m_maincpu->set_scanline_rgb32_callback(FUNC(tms_video_device::scanline_update));
	// m_maincpu->output_int().set(FUNC(artmagic_state::m68k_gen_int));
	m_maincpu->set_shiftreg_in_callback(FUNC(tms_video_device::to_shiftreg));
	m_maincpu->set_shiftreg_out_callback(FUNC(tms_video_device::from_shiftreg));

    TLC34076(config, m_tlc34076, tlc34076_device::TLC34076_6_BIT);

    screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_raw(MASTER_CLOCK_40MHz/6, 428, 0, 320, 313, 0, 256);
	screen.set_screen_update("maincpu", FUNC(tms34010_device::tms340x0_rgb32));

    m_maincpu->set_screen("screen");
}

ROM_START( tms_video )
	ROM_REGION16_LE( 0x100000, "maincpu", 0 )  /* 34010 code */
	ROM_LOAD16_BYTE( "prog.lo.bin", 0x00000, 0x80000, CRC(65cee452) SHA1(49259e8faf289d6d80769f6d44e9d61d15e431c6) )
	ROM_LOAD16_BYTE( "prog.hi.bin", 0x00001, 0x80000, CRC(5f4b0ca0) SHA1(57e9ed60cc0e53eeb4e08c4003138d3bdaec3de7) )
ROM_END

const tiny_rom_entry *tms_video_device::device_rom_region() const
{
	return ROM_NAME(tms_video);
}