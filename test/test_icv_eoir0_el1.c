

#include "sysreg/icv_eoir0_el1.h"



void test_unsafe_write_icv_eoir0_el1( void )
{
    unsafe_write_icv_eoir0_el1((union icv_eoir0_el1){ .intid=1 });
}


void test_safe_write_icv_eoir0_el1( void )
{
    safe_write_icv_eoir0_el1( .intid=1 );
}


