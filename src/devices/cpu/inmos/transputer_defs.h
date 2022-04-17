#ifndef MAME_CPU_INMOS_TRANSPUTER_DEFS_H
#define MAME_CPU_INMOS_TRANSPUTER_DEFS_H

#define OFFSET_MEMSTART 0x80000048

#define WORDMASK                ( 0xFFFFFFFC )
#define WORDLENGTH                      ( 32 )
#define BYTESPERWORD                    ( 4 )
#define BYTESELECTLENGTH                ( 2 )
//  2^(byteselectlength-1) < 4 <= 2^byteselectlength
//  if byteselectlength = 2, then 2^(2-1) < 4 <= 2^2
//  byteselectmask = 2^byteselectlength - 1
#define BYTESELECTMASK                  ( 2 * 2 - 1 )

// Oreg0 is Oreg before instruction execution.
// OregPrime is Oreg after execution. (e.g. m_OREG)
#define Oreg0                   ( m_OReg0 )
#define OregPrime               ( m_OREG )

// Wptr is Wdesc with byte selector 0.
#define Wptr                    ( m_WDESC & WORDMASK )

namespace transputer_bootstrap { 

    // The possible states for the bootstrap state machine.
    typedef enum
    {
        WAITING_FOR_LENGTH_BYTE,    // The initial state: Waiting for the bootstrap length byte from the UP link.
        WAITING_FOR_ADDRESS,        // Waiting for a DWORD address from the UP link.
        WAITING_FOR_DATA,           // Waiting for a DWORD of data from the UP link.
        WAITING_FOR_PROGRAM,        // Waiting for a bootstrap program from the UP link.
        CPU_SENDING_DATA,           // This CPU is sending data to a link.
        PEEK_BYTE_0,
        PEEK_BYTE_1,
        PEEK_BYTE_2,
        PEEK_BYTE_3
    } STATE;

    // The possible commands for the bootstrap state machine to execute.
    typedef enum
    {
        WAIT,   // Waiting for a command to come in from the UP link.
        LOAD,   // A program load is in progress.
        PEEK,   // A PEEK is in progress.
        POKE    // A POKE is in progress.
    } COMMAND;
}

#endif
