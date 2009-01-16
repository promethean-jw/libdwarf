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

*/

#include "config.h"
#include "dwarf_incl.h"
#include <stdio.h>
#include "dwarf_vars.h"
#include "dwarf_global.h"

int
dwarf_get_vars (
    Dwarf_Debug		dbg,
    Dwarf_Var		**vars,
    Dwarf_Signed       * ret_var_count,
    Dwarf_Error		*error
)
{
       return _dwarf_internal_get_pubnames_like_data(
                dbg,
		dbg->de_debug_varnames,
		dbg->de_debug_varnames_size,
                (Dwarf_Global **)vars, /* type punning,
                        Dwarf_Type is never a completed  type */
                ret_var_count,
                error,
                DW_DLA_VAR_CONTEXT,
                DW_DLE_DEBUG_VARNAMES_LENGTH_BAD,
                DW_DLE_DEBUG_VARNAMES_VERSION_ERROR);
}


int
dwarf_varname (
    Dwarf_Var		var_in,
    char      **        ret_varname,
    Dwarf_Error		*error
)
{
    Dwarf_Global var = (Dwarf_Global)var_in;
    if (var == NULL)
	{_dwarf_error(NULL, error, DW_DLE_VAR_NULL); return(DW_DLV_ERROR);}

    *ret_varname = (char *)(var->gl_name);
    return DW_DLV_OK;
}


int
dwarf_var_die_offset (
    Dwarf_Var		var_in,
    Dwarf_Off	*       returned_offset,
    Dwarf_Error		*error
)
{
    Dwarf_Global var = (Dwarf_Global)var_in;
    return
	dwarf_global_die_offset(var,
		returned_offset,
		error);
    
}


int
dwarf_var_cu_offset (
    Dwarf_Var		var_in,
    Dwarf_Off	*       returned_offset,
    Dwarf_Error		*error
)
{
    Dwarf_Global var = (Dwarf_Global)var_in;
    return
	dwarf_global_cu_offset(var,
		returned_offset, error);
}


int
dwarf_var_name_offsets (
    Dwarf_Var		var_in,
    char              **returned_name,
    Dwarf_Off		*die_offset,
    Dwarf_Off		*cu_offset,
    Dwarf_Error		*error
)
{
    Dwarf_Global var = (Dwarf_Global)var_in;
    return
	dwarf_global_name_offsets(var,
		returned_name, die_offset, cu_offset,
			error);
}
