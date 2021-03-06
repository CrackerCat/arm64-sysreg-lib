# Arm64 System Register Library

- [Introduction](#introduction)
- [Prerequisites](#prerequisites)
- [High-level functionality](#high-level-functionality)
  - [Bitfields](#bitfields)
  - [Safe values](#safe-values)
  - [Reads](#reads)
  - [Writes](#writes)
    - [Unsafe write](#unsafe-write)
    - [Safe write](#safe-write)
  - [Read-modify-write](#read-modify-write)
- [Building and testing the library](#building-and-testing-the-library)
  - [Step 1) Obtain AArch64 System Register XML](#step-1-obtain-aarch64-system-register-xml)
  - [Step 2) Build](#step-2-build)
  - [Step 3) Test](#step-3-test)
- [Known issues and limitations](#known-issues-and-limitations)
  

## Introduction
`arm64-sysreg-lib` is a header-only C library for reading/writing 64-bit Arm
registers, automatically generated by parsing the AArch64 System Register XML.

All functions are declared `static inline` and all file-scope variables are
declared `static const`, so modern optimizing compilers like `gcc` and `clang`
at `-O1` and above will collapse each call down down to an average of 1-4 A64
assembly instructions with no branches and the file-scope variables are stripped
out completely, using no storage.

The library includes compilation tests for all parsed system registers, all of
which have been successfully built using both `gcc` and `clang` and with the
`-Wall -Wextra -pedantic -Werror` flags passed to both compilers.


## Prerequisites

To build the library:
- Python 3.8+
- Beautiful Soup 4 (`pip3.8 install beautifulsoup4`)

To use the library in your own projects:
- C compiler supporting at least C99 with C11 extensions

Known working compilers:
* aarch64-none-elf-gcc (GCC) 7.2.0:
  - C99: `-Wall -Wextra -pedantic -Werror -std=c99 -O2`
  - C11: `-Wall -Wextra -pedantic -Werror -std=c11 -O2`

* Apple clang version 11.0.3 (clang-1103.0.32.62): *
  - C99: `-Wall -Wextra -pedantic -Werror -std=c99 -O2 --target=aarch64-none-elf`
  - C11: `-Wall -Wextra -pedantic -Werror -std=c11 -O2 --target=aarch64-none-elf`

\* Apple Clang also tested with `--target=aarch64-linux-gnu`.


## High-level functionality

### Bitfields
Availability: All system registers.

All parsed system registers define a `union` of the form:

    union sctlr_el1
    {
        u64 _;
        struct
        {
            u64 m : 1;
            u64 a : 1;
            u64 c : 1;
            u64 sa : 1;
            ...
        };
    };

The `._` member allows for raw access to the underlying register value while the
anonymous `struct` member allows for manipulation of the register's constituent
bit fields.


### Safe values
Availability: All writeable system registers.

The safe value for a writeable system register has all currently or previously
`RES1` fields set to `1` and all currently or previously `RES0` fields cleared
to `0`. These values are used by the `safe_write_<reg>()` convenience macros,
or you can use them yourself when manually constructing a value to write into
a register using `unsafe_write_<reg>()`.

Example:

    static const union sctlr_el3 SCTLR_EL3_SAFEVAL =
    {
        .res1_5_4 = 3,
        .eos = 1,
        .res1_16 = 1,
        .res1_18 = 1,
        .eis = 1,
        .res1_23 = 1,
        .res1_29_28 = 3,
    };

Some of these fields are currently `RES1` bits, such as `.res1_5_4` for bits
\[5:4\] and `.res1_16` for bit \[16\]. Setting these to `1` in the safe value
ensures portability when running on future CPU implementations where those bits
have been repurposed into new fields, as in these cases a value of `1` will give
the old behaviour while a value of `0` will give the new behaviour, and we
don't want to inadvertently enable the new behaviour by clearing them.

This is why the `.eos` and `.eis` fields are also set to `1`; these fields were
previosuly `RES1` bits but were repurposed into new fields in later revisions
of the architecture. The user can choose to clear these to `0` explicitly if
they want the new behaviour, but the library defaults to setting them to `1` in
the safe value and, by extension, the `safe_write_<reg>()` convenience macro.


### Reads
Availability: All readable system registers.

Read the current value of a system register into a field-accessible structure:

    /* C code */
    #include "sysreg/mpidr_el1.h"
    u64 foo( void )
    {
        return read_mpidr_el1().aff0;
    }

    /* Compiler output */
    mrs     x8, mpidr_el1
    and     x0, x8, #0xff
    ret


### Writes

#### Unsafe write
Availability: All writeable system registers.

Write a field-accessible structure into a system register. This function is
prefixed `unsafe_` to emphasise the fact that it has no provision for helping to ensure that any currently or previously `RES1` fields are set to `1`; when using
this function it is the responsibility of the programmer to ensure that any such
bits are set appropriately so as to ensure both correct behaviour and future portability. For this reason, it is recommended that you instead use
`safe_write_<reg>()` wherever possible, or use `<REG>_SAFEVAL` as a basis for
the value being constructed.

    /* C code */
    #include "sysreg/sctlr_el1.h"
    void foo( void )
    {
        union sctlr_el1 val = { .m=1, .c=1, .i=1 };
        unsafe_write_sctlr_el1(val);
    }

    /* Compiler output */
    mov     w8, #0x1005         // Danger! No RES1 bits set! See safe_write_<reg>()
    msr     sctlr_el1, x8
    ret

Use the `union`'s `._` member to write a raw value:

    /* C code */
    #include "sysreg/sctlr_el1.h"
    void foo( u64 raw )
    {
        union sctlr_el1 val = { ._=raw };
        unsafe_write_sctlr_el1(val);
    }

    /* Compiler output */
    msr     sctlr_el1, x0
    ret


#### Safe write
Availability: All writeable system registers.

Use `safe_write_<reg>()` to set a variadic list of fields. Any fields not
specified in the variadic list that are currently or previously `RES1` will be
set to `1`, and all other fields will be cleared to `0`.

For example:

    /* C code */
    #include "sysreg/sctlr_el1.h"
    void foo( void )
    {
        safe_write_sctlr_el1( .m=1, .c=1, .i=1 );
    }

    /* compiler output */
    mov     w8, #0x1985
    movk    w8, #0x30d0, lsl #16
    msr     sctlr_el1, x8
    ret

Note how while we only specified `.m=1`, `.c=1`, and `.i=1`, the value written
to `SCTLR_EL1` also has all currently or previously `RES1` fields set to `1`,
such as `.itd=1`, `.sed=1`, `.eos=1`, etc.

Repeating the same, but this time explicitly clearing one of those currently or previously `RES1` fields to `0` in the variadic list:

    /* in C */
    #include "sysreg/sctlr_el1.h"
    void foo( void )
    {
        safe_write_sctlr_el1( .m=1, .c=1, .i=1, .itd=0 );
    }

    /* compiler output */
    mov     w8, #0x1905
    movk    w8, #0x30d0, lsl #16
    msr     sctlr_el1, x8
    ret

Here we can see bit \[7\] corresponding to `.itd` in the first `MOV` has been
cleared; the value moved into `w8` is now `0x1905` vs `0x1985` in the earlier
example.


### Read-modify-write
Availability: All system registers that are both readable and writeable.

Use `read_modify_write_<reg>()` to read the current value of a system register,
set a variadic list of fields in that value, then write the result back.

    /* in C */
    void foo( void )
    {
        read_modify_write_sctlr_el1( .m=1, .c=1, .i=1 );
    }

    /* compiler output */
    mrs     x8, sctlr_el1
    mov     w9, #0x1005
    orr     x8, x8, x9
    msr     sctlr_el1, x8
    ret

NOTE: Many AArch64 system registers have architecturally UNKNOWN values at
reset, meaning you should *not* perform a read-modify-write sequence when first
initializing them. Instead, use `safe_write_<reg>()` to guarantee that all
currently or previously `RES1` fields are set to `1` and all other unspecified
fields are cleared to `0`.


## Building and testing the library

NOTE: The library has already been built for you using the June 2020 release of
the AArch64 System Register XML (SysReg_xml_v86A-2020-06). You can simply add
`-I/path/to/arm64-sysreg-lib/include` to your compiler flags to begin using the
library in your own projects straight away. You can also run the `run-tests.py`
script to build the compilation tests using your chosen compiler. More detailed
instructions for running the compilation tests can be found at the end of this
document.
 

### Step 1) Obtain AArch64 System Register XML

Download and extract the AArch64 System Register XML from the
[Arm A-Profile CPU architecture exploration tools page](https://developer.arm.com/architectures/cpu-architecture/a-profile/exploration-tools).

Alternatively, use `curl` to download the Armv8.6-A XML published in June 2020:

    curl -O https://developer.arm.com/-/media/developer/products/architecture/armv8-a-architecture/2020-06/SysReg_xml_v86A-2020-06.tar.gz
    tar xf SysReg_xml_v86A-2020-06.tar.gz

### Step 2) Build

Run the provided `run-build.py` script, pointing it at the AArch64 System Register
XML downloaded and extracted earlier:

    python3.8 run-build.py /path/to/SysReg_xml_v86A-2020-06

### Step 3) Test

Run the provided `run-tests.py` script, pointing it at your chosen compiler:

    python3.8 run-tests.py [--keep] COMPILER_PATH [COMPILER_FLAGS]

For example:

    python3.8 run-tests.py /path/to/aarch64-none-elf-gcc 

It is assumed the compiler uses the same switches as `gcc` and `clang`, and the
script always invokes the compiler with the following flags:

    -Wall -Wextra -pedantic -Werror

You may pass additional flags to the `run-tests.py` script which will be passed
in turn to the compiler, for example:

    python3.8 run-tests.py /path/to/aarch64-none-elf-gcc -std=c99 -O3  

If no `-std` flag is provided, the script defaults to `-std=c11`.

If no `-O` flag is provided, the script defaults to `-O2`.

If the compiler path contains substring `clang` and no `--target` flag is provided,
the script defaults to `--target=aarch64-none-elf`.

By default the script will cleanup the generated `.o` object files after finishing
the test run; pass the `--keep` flag before the compiler path if you wish to keep
these files.

## Known issues and limitations

This library is still in development and is not yet able to parse all files
included in the AArch64 System Register XML.

Of the 485 `AArch64-*` files included in the `SysReg_xml_v86A-2020-06` release
that actually describe system register encodings:
- 126 are skipped as they correspond to instructions that use the system register
  encoding space (`ic`, `dc`, `tlbi`, `at`, `cfp`, `cpp`, and `dvp`)
- 263 successfully build
- 96 fail to parse

Of the 96 that fail to parse:
- 43 fail as their fields vary, such a when a bit in another register is set to `1`
- 19 fail due to being an arrayed register (`<n>`)
- 23 fail due to having arrayed fields (`<m>`, `<n>`, or `<x>`)
- 11 fail due to having variable length fields

Support for these registers will be added in a future release.

Not counted above are external system registers such as `GICD_*` and `GICR_*`,
support for which will also be added later.
