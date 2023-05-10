// license:BSD-3-Clause
// copyright-holders:Katherine Rohl
/***************************************************************************

    transputer.h
    Header for the INMOS transputer architecture.

***************************************************************************/

#ifndef MAME_CPU_INMOS_TRANSPUTER_BOOTSTRAP_H
#define MAME_CPU_INMOS_TRANSPUTER_BOOTSTRAP_H

#pragma once

#include "transputer_private.h"

/********************************************
 *** Bootstrapping
 * 
 *  Upon reset, the CPU checks the state of the BootFromRom pin.
 *  If the pin is low, the microcode will enter a bootstrap procedure after two clock cycles.
 * 
 *  - The CPU awaits a command from any link. 
 *  - The command consists of one length byte followed by up to 255 bytes of data.
 *  - After the length byte is received on one link, incoming data on other links is ignored until a process requests it.
 *  - After the last bootstrap byte is received, incoming data on that link is ignored until a process requests it.
 * 
 *** Bootstrap Commands
 * 
 *  The first byte received on the link is interpreted as a command or length byte.
 * 
 *  - $00: POKE WORD [address] WORD [data]
 *      The data word is inserted into memory at the specified address.
 *      The CPU then waits for another bootstrap command.
 *  - $01: PEEK WORD [address]
 *      The word at the specified address is read out to the link that sent the bootstrap command.
 *      The CPU then waits for another bootstrap command.
 *  - $02 to $FF: BOOTSTRAP
 *      Instructs the CPU to read a certain number of bytes from the link into a buffer at [MemStart].
 *      The workspace pointer is initialized to the first byte after the bootstrap program.
 *      The CPU begins executing a low-priority process at [MemStart].
 */

// Perform a bootstrap step this clock.
void transputer_cpu_device::bootstrap_step()
{
    // Behavior for when there's a byte in the receive buffer.
    if(m_links[0].rx_byte_present)
    {
        uint8_t data = m_links[0].rx_buffer;

        switch(m_bootstrap_status.command)
        {
            case transputer_bootstrap::COMMAND::LOAD:
            {
                LOG("current COMMAND is LOAD. %d bytes remaining\n", m_bootstrap_status.length_remaining-1);

                // Store data in memory until we're out of length_remaining.
                memory.write_byte(OFFSET_MEMSTART + (m_bootstrap_status.current_buffer_offset++), data);

                if(--m_bootstrap_status.length_remaining == 0)
                {
                    bootstrap_begin_execution();
                }
                break;
            }   
            case transputer_bootstrap::COMMAND::PEEK:
            {
                //LOG("current COMMAND is PEEK.\n", m_bootstrap_status.length_remaining);
                switch(m_bootstrap_status.state)
                {
                    case transputer_bootstrap::STATE::WAITING_FOR_ADDRESS:
                    {
                        LOG("got a PEEK address byte. %d bytes remaining\n", m_bootstrap_status.length_remaining-1);
                        
                        uint32_t tmp_data = data;
                        tmp_data <<= (8 * (4-m_bootstrap_status.length_remaining--));
                        m_bootstrap_status.command_addr |= tmp_data;

                        if(m_bootstrap_status.length_remaining == 0)
                        {
                            // We got a complete address. Read memory and dump to the link.
                            LOG("got a complete PEEK address. reading memory at %08X\n", m_bootstrap_status.command_addr);
                            m_bootstrap_status.state = transputer_bootstrap::STATE::PEEK_BYTE_0;
                            m_bootstrap_status.command_data = memory.read_dword(m_bootstrap_status.command_addr);
                        }
                        break;
                    }
                    default:
                        LOG("bootstrap state is invalid\n");
                        break; // ignore incoming data, we're outputting data.
                }
                break;
            }
            case transputer_bootstrap::COMMAND::POKE:
            {
                switch(m_bootstrap_status.state)
                {
                    case transputer_bootstrap::STATE::WAITING_FOR_ADDRESS:
                    {
                        LOG("got a POKE address byte. %d bytes remaining\n", m_bootstrap_status.length_remaining-1);
                        uint32_t tmp_data = data;
                        tmp_data <<= (8 * (4-m_bootstrap_status.length_remaining--));
                        m_bootstrap_status.command_addr |= tmp_data;

                        if(m_bootstrap_status.length_remaining == 0)
                        {
                            // We got a complete address. Advance to waiting for data.
                            LOG("Bootstrap POKE: got a complete address %08X\n", m_bootstrap_status.command_addr);
                            m_bootstrap_status.state = transputer_bootstrap::WAITING_FOR_DATA;
                            m_bootstrap_status.length_remaining = 4;
                        }
                        break;
                    }
                    case transputer_bootstrap::STATE::WAITING_FOR_DATA:
                    {
                        LOG("got a POKE data byte. %d bytes remaining\n", m_bootstrap_status.length_remaining-1);
                        uint32_t tmp_data = data;
                        tmp_data <<= (8 * (4-m_bootstrap_status.length_remaining--));
                        m_bootstrap_status.command_data |= tmp_data;

                        if(m_bootstrap_status.length_remaining == 0)
                        {
                            // We got a complete data word. Store it in RAM.
                            LOG("Bootstrap POKE: addr %08X <- data %08X\n", m_bootstrap_status.command_addr, m_bootstrap_status.command_data);
                            memory.write_dword(m_bootstrap_status.command_addr, m_bootstrap_status.command_data);

                            // Reset for the next bootstrap command.
                            m_bootstrap_status.state = transputer_bootstrap::STATE::WAITING_FOR_LENGTH_BYTE;
                            m_bootstrap_status.command = transputer_bootstrap::COMMAND::WAIT;
                        }
                        break;
                    }
                    default:
                        break; // ignore incoming data, we're outputting data.
                }
                break;
            }
            default:
                // We are not in a command, so this is a length byte.
                if(data == 0x00)
                {
                    // Start a POKE.
                    LOG("bootstrap: starting a POKE command\n");
                    m_bootstrap_status.command = transputer_bootstrap::COMMAND::POKE;
                    m_bootstrap_status.length_remaining = 4;
                    m_bootstrap_status.state = transputer_bootstrap::STATE::WAITING_FOR_ADDRESS;
                    m_bootstrap_status.command_addr = 0;
                    m_bootstrap_status.command_data = 0;
                }
                else if(data == 0x01)
                {
                    // Start a PEEK.
                    LOG("bootstrap: starting a PEEK command\n");
                    m_bootstrap_status.command = transputer_bootstrap::COMMAND::PEEK;
                    m_bootstrap_status.length_remaining = 4;
                    m_bootstrap_status.state = transputer_bootstrap::STATE::WAITING_FOR_ADDRESS;
                    m_bootstrap_status.command_addr = 0;
                    m_bootstrap_status.command_data = 0;
                }
                else
                {
                    // Start receiving a program.
                    LOG("bootstrap: starting a LOAD command\n");
                    m_bootstrap_status.command = transputer_bootstrap::COMMAND::LOAD;
                    m_bootstrap_status.length_remaining = data;
                    m_bootstrap_status.current_buffer_offset = 0;
                    m_bootstrap_status.state = transputer_bootstrap::STATE::WAITING_FOR_PROGRAM;
                    m_bootstrap_status.code_length = data;
                }
                break;
        }
    }

    // If the transmitter is ready, do we need to send a byte?
    if(m_links[0].tx_is_ready())
    {
        switch(m_bootstrap_status.command)
        {
            case transputer_bootstrap::COMMAND::PEEK:
            {
                switch(m_bootstrap_status.state)
                {
                    case transputer_bootstrap::STATE::PEEK_BYTE_0:
                    {
                        // Send 4 bytes down link0.
                        if(m_links[0].tx_is_ready())
                        {
                            LOG("sending PEEK byte 0\n");
                            m_links[0].send_byte(m_bootstrap_status.command_data & 0xff, serial_get_next_edge());
                            m_bootstrap_status.state = transputer_bootstrap::STATE::PEEK_BYTE_1;

                            m_bootstrap_status.command_data >>= 8;
                        }
                        break;
                    }
                    case transputer_bootstrap::STATE::PEEK_BYTE_1:
                    {
                        // Send 4 bytes down link0.
                        if(m_links[0].tx_is_ready())
                        {
                            LOG("sending PEEK byte 1\n");
                            m_links[0].send_byte(m_bootstrap_status.command_data & 0xff, serial_get_next_edge());
                            m_bootstrap_status.state = transputer_bootstrap::STATE::PEEK_BYTE_2;

                            m_bootstrap_status.command_data >>= 8;
                        }
                        break;
                    }
                    case transputer_bootstrap::STATE::PEEK_BYTE_2:
                    {
                        // Send 4 bytes down link0.
                        if(m_links[0].tx_is_ready())
                        {
                            LOG("sending PEEK byte 2\n");
                            m_links[0].send_byte(m_bootstrap_status.command_data & 0xff, serial_get_next_edge());
                            m_bootstrap_status.state = transputer_bootstrap::STATE::PEEK_BYTE_3;

                            m_bootstrap_status.command_data >>= 8;
                        }
                        break;
                    }
                    case transputer_bootstrap::STATE::PEEK_BYTE_3:
                    {
                        // Send 4 bytes down link0.
                        if(m_links[0].tx_is_ready())
                        {
                            LOG("sending PEEK byte 3\n");
                            m_links[0].send_byte(m_bootstrap_status.command_data & 0xff, serial_get_next_edge());

                            // Reset for the next bootstrap command.
                            m_bootstrap_status.state = transputer_bootstrap::STATE::WAITING_FOR_LENGTH_BYTE;
                            m_bootstrap_status.command = transputer_bootstrap::COMMAND::WAIT;
                        }
                        break;
                    }
                    default: break;
                }
            }
            default: break;
        }
    }
}

void transputer_cpu_device::bootstrap_begin_execution()
{
    LOG("Bootstrap program received, beginning execution\n");
    // 9.2.1 contains the info on the boot-up processor state.

    m_AREG = m_IPTR;            // Areg = old IPTR
    m_BREG = m_WDESC;           // Breg = old WDESC
    m_CREG = LINK_OUTPUT_0;     // Creg = link that sent the program
    m_IPTR = OFFSET_MEMSTART;   // Iptr = MEMSTART

    // Wdesc = ffw BITOR 1
    // where ffw is the smallest word address >= MEMSTART + CODELENGTH
    m_WDESC = ((OFFSET_MEMSTART + m_bootstrap_status.code_length + 4) & 0xFFFFFFFC) | 1;

    m_cpu_state = CPU_RUNNING;
}

#endif