/*
Copyright (c) 1994-9 Silicon Graphics, Inc.

    Permission to use, copy, modify, distribute, and sell this software and 
    its documentation for any purpose is hereby granted without fee, provided
    that (i) the above copyright notice and this permission notice appear in
    all copies of the software and related documentation, and (ii) the name
    "Silicon Graphics" or any other trademark of Silicon Graphics, Inc.  
    may not be used in any advertising or publicity relating to the software
    without the specific, prior written permission of Silicon Graphics, Inc.

    THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
    EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
    WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  

    IN NO EVENT SHALL SILICON GRAPHICS, INC. BE LIABLE FOR ANY SPECIAL, 
    INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
    OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
    WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
    LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
    OF THIS SOFTWARE.

	pro_alloc.h

	allocation routines which are not part of the public
	dwarf interface (but are part of the libdwarf internals)
	are declared here.

	$Revision: 1.3 $    $Date: 1999/03/05 22:00:16 $
*/

Dwarf_Ptr _dwarf_p_get_alloc(Dwarf_P_Debug, Dwarf_Unsigned);

void _dwarf_p_dealloc (Dwarf_P_Debug dbg, Dwarf_Small *ptr);
