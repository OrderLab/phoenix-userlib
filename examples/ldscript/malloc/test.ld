 /* PHDRS { */
     /* phxseg PT_LOAD; */
 /* } */

SECTIONS {
    /* . = SEGMENT_START("phxseg", .); */
    /* .phxdata : ALIGN(0x1693) { *(.data) } */
    /* .phxdata : ALIGN(0x2000) { *(.data) } */
    .phx.data : ALIGN(0x1000) { *(.data) }
    .phx.bss : ALIGN(0x1000) { *(.bss) }
}
