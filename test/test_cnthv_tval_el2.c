

#include "sysreg/cnthv_tval_el2.h"


u64 test_read_cnthv_tval_el2( void )
{
    return read_cnthv_tval_el2().timervalue;
}


void test_unsafe_write_cnthv_tval_el2( void )
{
    unsafe_write_cnthv_tval_el2((union cnthv_tval_el2){ .timervalue=1 });
}


void test_safe_write_cnthv_tval_el2( void )
{
    safe_write_cnthv_tval_el2( .timervalue=1 );
}


void test_read_modify_write_cnthv_tval_el2( void )
{
    read_modify_write_cnthv_tval_el2( .timervalue=1 );
}

