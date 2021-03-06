

#include "sysreg/pmscr_el2.h"


u64 test_read_pmscr_el2( void )
{
    return read_pmscr_el2().e0hspe;
}


void test_unsafe_write_pmscr_el2( void )
{
    unsafe_write_pmscr_el2((union pmscr_el2){ .e0hspe=1 });
}


void test_safe_write_pmscr_el2( void )
{
    safe_write_pmscr_el2( .e0hspe=1 );
}


void test_read_modify_write_pmscr_el2( void )
{
    read_modify_write_pmscr_el2( .e0hspe=1 );
}

