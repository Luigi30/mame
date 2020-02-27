// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    TMS9918A Video Card by J.B. Langston

**********************************************************************/

#include "emu.h"
#include "vdp_video.h"

#include "video/tms9928a.h"

#define TMS9918A_TAG	"tms9918a"
#define SCREEN_TAG		"screen"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vdp_video_device

class vdp_video_device : public device_t, public device_rc2014_card_interface
{
public:
	// construction/destruction
	vdp_video_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock);

protected:
	// device-specific overrides
	virtual ioport_constructor device_input_ports() const override;
	virtual void device_start() override;
	virtual void device_reset() override;

	virtual void device_add_mconfig(machine_config &config) override;

private:
	// object finder
	required_device<tms9918a_device> m_vdp;
	rc2014_slot_device *m_slot;
};

DEFINE_DEVICE_TYPE_PRIVATE(RC2014_VDP_VIDEO, device_rc2014_card_interface, vdp_video_device, "vdp_video", "TMS9918A Video Card")


//**************************************************************************
//  CONFIGURATION SETTINGS
//**************************************************************************

    /* Jumpers that enable/disable address lines. */

	// TODO: Find a better way to describe these.
static INPUT_PORTS_START(vdp_video)
	PORT_START("J4")
	PORT_DIPNAME(0x7, 0x4, "I/O Port Base") PORT_DIPLOCATION("J4:1,2,3,4")
	PORT_DIPSETTING(0x0, "00H")
    PORT_DIPSETTING(0x1, "20H")
    PORT_DIPSETTING(0x2, "40H")
	PORT_DIPSETTING(0x3, "60H")
	PORT_DIPSETTING(0x4, "80H")	// MSX
	PORT_DIPSETTING(0x5, "A0H")	// ColecoVision
	PORT_DIPSETTING(0x6, "C0H")
	PORT_DIPSETTING(0x7, "E0H")

	PORT_START("J6")
	PORT_DIPNAME(0x1, 0x1, "Decode I/O Range") PORT_DIPLOCATION("J6:1")
	PORT_DIPSETTING(0x0, "00h-0Fh")
    PORT_DIPSETTING(0x1, "10h-1Fh")

	PORT_START("JP1")
	PORT_DIPNAME(0x1, 0x1, "Decode I/O Bits 1 and 2") PORT_DIPLOCATION("JP1:1")
	PORT_DIPSETTING(0x0, "No")
    PORT_DIPSETTING(0x1, "Yes")

	PORT_START("JP2")
	PORT_DIPNAME(0x1, 0x1, "Decode I/O Bit 3") PORT_DIPLOCATION("JP2:1")
	PORT_DIPSETTING(0x0, "No")
    PORT_DIPSETTING(0x1, "Yes")
    
	PORT_START("JP4")
	PORT_DIPNAME(0x1, 0x1, "CPU Interrupt Line") PORT_DIPLOCATION("JP4:1")
	PORT_DIPSETTING(0x0, "NMI")
    PORT_DIPSETTING(0x1, "INT")
INPUT_PORTS_END


//**************************************************************************
//  DEVICE DEFINITION
//**************************************************************************

//-------------------------------------------------
//  poly_16k_ram_device - constructor
//-------------------------------------------------

vdp_video_device::vdp_video_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: device_t(mconfig, RC2014_VDP_VIDEO, tag, owner, clock)
	, device_rc2014_card_interface(mconfig, *this)
	, m_vdp(*this, TMS9918A_TAG)
	, m_slot(nullptr)
{
}



//-------------------------------------------------
//  device_input_ports - input port construction
//-------------------------------------------------

ioport_constructor vdp_video_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(vdp_video);
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void vdp_video_device::device_start()
{

}

void vdp_video_device::device_reset()
{
	uint8_t io_base = (ioport("J4")->read() << 4) * 2;

	m_slot = dynamic_cast<rc2014_slot_device *>(owner());
	cpu_device &cpu = m_slot->cpu();
	address_space &space = cpu.space(AS_IO);

	space.install_readwrite_handler(io_base+0x18, io_base+0x18, read8smo_delegate(*m_vdp, FUNC(tms9918a_device::vram_read)), write8smo_delegate(*m_vdp, FUNC(tms9918a_device::vram_write)));
	space.install_readwrite_handler(io_base+0x19, io_base+0x19, read8smo_delegate(*m_vdp, FUNC(tms9918a_device::register_read)), write8smo_delegate(*m_vdp, FUNC(tms9918a_device::register_write)));

	if(ioport("JP4")->read() == 0)
	{
		m_vdp->int_callback().set_inputline(cpu, INPUT_LINE_NMI);
	}
	else
	{
		m_vdp->int_callback().set_inputline(cpu, INPUT_LINE_IRQ0);
	}
}

void vdp_video_device::device_add_mconfig(machine_config &config)
{
	m_slot = dynamic_cast<rc2014_slot_device *>(owner());

	SCREEN(config, SCREEN_TAG, SCREEN_TYPE_RASTER);
	TMS9918A(config, m_vdp, 10.738635_MHz_XTAL);
	m_vdp->set_screen(SCREEN_TAG);
	m_vdp->set_vram_size(0x4000);

}