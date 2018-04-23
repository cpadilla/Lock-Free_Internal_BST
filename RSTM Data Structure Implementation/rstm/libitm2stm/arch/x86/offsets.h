/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM for licensing information
 */

#ifndef ITM_LIBITM_ARCH_X86_OFFSETS_H
#define ITM_LIBITM_ARCH_X86_OFFSETS_H

// -----------------------------------------------------------------------------
// Defines some struct offsets that we use in _ITM_beginTransaction. This should
// be a configure or build-time generated header, but we don't really have the
// resources set up for that. Instead, we statically check these using the
// CheckOffsets.h header.
// -----------------------------------------------------------------------------

#define TRANSACTION_INNER_          8
#define TRANSACTION_FREE_SCOPES_    12
#define SCOPE_ABORTED_              32
#define NODE_NEXT_                  92

#endif // ITM_LIBITM_ARCH_X86_OFFSETS_H
