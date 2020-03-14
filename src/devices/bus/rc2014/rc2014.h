// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/**********************************************************************

    RC2014 bus emulation

**********************************************************************

	Pinout ASCII art here

**********************************************************************/

#ifndef MAME_BUS_RC2014_RC2014_H
#define MAME_BUS_RC2014_RC2014_H

#pragma once

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class rc2014_bus_device;

// ======================> device_rc2014_card_interface

class device_rc2014_card_interface : public device_interface
{
	friend class rc2014_bus_device;
	template <class ElementType> friend class simple_list;

public:
	device_rc2014_card_interface *next() const { return m_next; }

	virtual void rc2014_m1_w(int state) {}

	// // interrupts
	virtual void rc2014_int_w(int state) { }
	virtual void rc2014_nmi_w(int state) { }

	// // memory access
	virtual uint8_t rc2014_rd_r(offs_t offset) { return 0xff; }
	virtual void rc2014_wr_w(offs_t offset, uint8_t data) { }

	// // I/O access
	virtual uint8_t rc2014_io_r(offs_t offset) { return 0xff; }
	virtual void rc2014_io_w(offs_t offset, uint8_t data) { }

	// // reset
	virtual void rc2014_reset_w(int state) { }

protected:
	// construction/destruction
	device_rc2014_card_interface(const machine_config &mconfig, device_t &device);

	virtual void interface_pre_start() override;

	rc2014_bus_device  *m_bus;

private:
	device_rc2014_card_interface *m_next;
};



// ======================> rc2014_bus_device

class rc2014_bus_device : public device_t
{
public:
	// construction/destruction
	rc2014_bus_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
	~rc2014_bus_device() { m_device_list.detach_all(); }

	void add_card(device_rc2014_card_interface *card);

	uint8_t rd_r(offs_t offset);
	void wr_w(offs_t offset, uint8_t data);

	uint8_t io_r(offs_t offset);
	void io_w(offs_t offset, uint8_t data);

	// DECLARE_WRITE_LINE_MEMBER( m1_w ) 	{ m_write_m1(state); }
	// DECLARE_WRITE_LINE_MEMBER( mreq_w ) { m_write_mreq(state); }
	// DECLARE_WRITE_LINE_MEMBER( ireq_w ) { m_write_ireq(state); }
	// DECLARE_WRITE_LINE_MEMBER( wr_w ) 	{ m_write_wr(state); }
	// DECLARE_WRITE_LINE_MEMBER( rd_w ) 	{ m_write_rd(state); }
	// DECLARE_WRITE_LINE_MEMBER( usr1_w ) { m_write_usr1(state); }
	// DECLARE_WRITE_LINE_MEMBER( usr2_w ) { m_write_usr2(state); }
	// DECLARE_WRITE_LINE_MEMBER( usr3_w ) { m_write_usr3(state); }
	// DECLARE_WRITE_LINE_MEMBER( usr4_w ) { m_write_usr4(state); }

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;

	virtual void device_resolve_objects() override;

private:
	// devcb_write_line m_out_irq_cb;
	// devcb_write_line m_out_nmi_cb;
	// devcb_write_line	m_write_m1;
	// devcb_write_line	m_write_mreq;
	// devcb_write_line	m_write_ireq;
	// devcb_write_line	m_write_wr;
	// devcb_write_line	m_write_rd;
	// devcb_write_line	m_write_usr1;
	// devcb_write_line	m_write_usr2;
	// devcb_write_line	m_write_usr3;
	// devcb_write_line	m_write_usr4;

	simple_list<device_rc2014_card_interface> m_device_list;
};


// ======================> rc2014_slot_device

class rc2014_slot_device : public device_t, public device_single_card_slot_interface<device_rc2014_card_interface>
{
public:
	// construction/destruction
    template <typename T, typename U, typename V>
    rc2014_slot_device(machine_config const &mconfig, char const *tag, device_t *owner, T &&bus, U &&cpu, V &&opts, char const *dflt)
        : rc2014_slot_device(mconfig, tag, owner, DERIVED_CLOCK(1, 1))
    {
        option_reset();
        opts(*this);
        set_default_option(dflt);
        set_fixed(false);
        set_bus(std::forward<T>(bus));
        set_cpu_tag(std::forward<U>(cpu));
    }
	rc2014_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	template <typename T> void set_bus(T &&tag) { m_bus.set_tag(std::forward<T>(tag)); }

	template <class Object> void set_cpu_tag(Object &&tag) { m_cpu.set_tag(std::forward<Object>(tag)); }
	cpu_device &cpu() const { return *m_cpu; }

	DECLARE_WRITE_LINE_MEMBER( nmi_w );
	DECLARE_WRITE_LINE_MEMBER( irq_w );

protected:
	// device-level overrides
	virtual void device_start() override;

private:
	required_device<rc2014_bus_device> m_bus;
	required_device<cpu_device> m_cpu;
};


// device type definition
DECLARE_DEVICE_TYPE(RC2014_BUS,  rc2014_bus_device)
DECLARE_DEVICE_TYPE(RC2014_SLOT, rc2014_slot_device)

#endif // MAME_BUS_RC2014_RC2014_H
