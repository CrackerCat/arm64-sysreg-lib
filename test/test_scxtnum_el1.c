

#include "sysreg/scxtnum_el1.h"


u64 test_read_scxtnum_el1( void )
{
    return read_scxtnum_el1().Software_Context_Number;
}


void test_unsafe_write_scxtnum_el1( void )
{
    unsafe_write_scxtnum_el1((union scxtnum_el1){ .Software_Context_Number=1 });
}


void test_safe_write_scxtnum_el1( void )
{
    safe_write_scxtnum_el1( .Software_Context_Number=1 );
}


void test_read_modify_write_scxtnum_el1( void )
{
    read_modify_write_scxtnum_el1( .Software_Context_Number=1 );
}

