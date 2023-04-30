 /* PHDRS { */
     /* phxseg PT_LOAD; */
 /* } */

SECTIONS {
    /* . = SEGMENT_START("phxseg", .); */
    /* .phxdata : ALIGN(0x1693) { *(.data) } */
    /* .phxdata : ALIGN(0x2000) { *(.data) } */
    .phxdata : { *(.data) }
    .phxbss : { *(.bss) }
}
