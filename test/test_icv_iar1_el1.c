

#include "sysreg/icv_iar1_el1.h"


u64 test_read_icv_iar1_el1( void )
{
    return read_icv_iar1_el1().intid;
}




