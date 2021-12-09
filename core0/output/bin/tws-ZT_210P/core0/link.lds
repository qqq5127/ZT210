/* --------------- reg type --------------- */
/* --------------- ram type --------------- */
/* --------------- ext_ram type --------------- */
/* --------------- rom type --------------- */
/* --------------- end address type --------------- */
/* DTOP */
/* BT */
/* DSP */
/* FLASH 2M */
/* PSRAM DDR 4M */
/* Image header */
/**
 * flash start ->  +-----------+
 *                 |   PBL     | --> 4K (1 sectors)
 *                 +-----------+
 *                 |   SBL     | --> 60K (15 sectors)
 *                 +-----------+
 *                 |   BCP     | --> 480K (120 sectors)
 *                 +-----------+
 *                 |   ACP     | --> 320K (80 sectors)
 *                 +-----------+
 *                 |   DCP     | --> 384K (96 sectors)
 *                 +-----------+
 *                 |   FOTA    | --> 644K (161 sectors)
 *                 +-----------+
 *                 |   TONE    | --> 100K (25 sectors)
 *                 +-----------+
 *                 |    ANC    | --> 8K (2 sectors)
 *                 +-----------+
 *                 |    KV     | --> 32K (8 sectors)
 *                 +-----------+
 *                 | BOOT MAP  | --> 8K (2 sectors)
 *                 +-----------+
 *                 |    OEM    | --> 4K (1 sectors)
 *                 +-----------+
 *                 |    CAL    | --> 4K (1 sectors)
 * flash end   ->  +-----------+
 */
/**
 * DTOP rom total 32K
 */
/**
 * BT rom total 1M (512K*2)
 */
/**
 * DTOP iram total 64K (32K*2 + 64K)
 */
/**
 * BT iram total 288K (128K + 64K*2 + 32K)
 */
/* Share memory */
/* bt share data is in WIC_DEPENDENCE_SCRATCH */
/* Generic transmission */
/* DTOP can directly use API call, 0 indicates give up to use share memory */
/* The buffer head is used for GENERIC_TRANSMISSION_CTRL_SIZE */
/**
 * ring for rpc data transport origin definiation
 * At end of th dcp iram
 * AUDIO push to   DTOP 30K
 * BT    push to   DTOP 1K
 * BT    pull from DTOP 1K
 * Total size           32k
 */
/* ipc message */
/* Flash code memory (512K for DTOP) */
/* BT patch TBL */
OUTPUT_ARCH( "riscv" )
ENTRY( _start )
__stack_size = DEFINED(__stack_size) ? __stack_size : 4K;
MEMORY
{
    flash : ORIGIN = (((((0x4000000 + (0x1000 * 1)) + (0x1000 * 15)) + (0x1000 * 120)) + (0x1000 * 80)) + 0x20), LENGTH = ((0x1000 * 96) - 0x20)
    ram : ORIGIN = (0x02000000), LENGTH = (((((0x0200FFFF - 0x02000000) + 1) + ((0x0201FFFF - 0x02010000) + 1) + ((0x02027FFF - 0x02020000) + 1) + ((0x0202FFFF - 0x02028000) + 1)) - (0x40 + (0x800 + 0x800 + 0x800) + (0x7800 + 0x400 + 0x400) + 0x800) - 0x7000))
}
SECTIONS
{
    .init :
    {
        KEEP (*(SORT_NONE(.init)))
        . = ALIGN(4);
    } >flash
    .iram_text :
    {
        . = ALIGN(4);
        *libpower_mgnt.a:dev_pm.o(.text .text.* .rodata .rodata.*)
        *libfreertos.a:*.o(.text.os_sys_restore)
        *lib*driver.a:flash_special.o(.text.flash_gd_set_quad_mode)
        *lib*driver.a:flash.o(.text.flash_special_enable_quad_mode .text.flash_special_set_cache_mode)
        *lib*driver.a:flash.o(.text.flash_special_read_status_reg2 .text.flash_special_read_status_reg1)
        *lib*driver.a:flash.o(.text.flash_special_write_enable .text.flash_special_write_status_reg1 .text.flash_special_wait_wip)
        *lib*driver.a:iot_flash.o(.text.iot_flash_enable_quad_mode .text.iot_flash_set_cache_mode)
        *lib*driver.a:sfc.o(.text.sfc_init .text.sfc_set_io_map .text.sfc_set_edge)
        *lib*driver.a:sfc.o(.text.sfc_set_suspend_resume_param .text.sfc_set_dummy_num .text.sfc_set_endian_mode)
        *lib*driver.a:sfc.o(.text.sfc_set_cache_mode .text.sfc_qspi_cmd_cache_sel .text.sfc_qspi_cmd_wip_sel)
        *lib*driver.a:sfc.o(.text.sfc_qspi_cmd_resume_sel .text.sfc_qspi_cmd_suspend_sel)
        *lib*driver.a:sfc.o(.text.sfc_send_read_cmd .text.sfc_set_wtime .text.sfc_set_cmd_swm)
        *lib*driver.a:sfc.o(.text.sfc_set_cmd .text.sfc_start_cmd .text.sfc_is_cmd_busy)
        *lib*driver.a:sfc.o(.text.sfc_get_read_data .text.sfc_send_write_cmd .text.sfc_set_write_data)
        *lib*driver.a:iot_lpm_entry.o(.text .text.* .rodata .rodata.*)
        *lib*driver.a:*.o(.text .text.machine_*_interrupt_handler)
        *lib*driver.a:apb.o(.text .text.* .rodata .rodata.*)
        *lib*driver.a:pmm.o(.text.pmm_switch_to_dcdc_1p8 .text.pmm_live_dcdc_ldo_config .text.pmm_debug_mode_enable)
        *lib*driver.a:pmm.o(.text.pmm_bt_timing_gen_soft_rst .text.pmm_sleep_control)
        *lib*driver.a:pin.o(.text .text.* .rodata .rodata.*)
        *(.iram_text .iram_text.*)
        *(.iram)
        *libfreertos.a:*.o(.text .text.* .rodata .rodata.*)
        *(.iram_rodata .iram_rodata.*)
        . = ALIGN(4);
    } >ram AT>flash
    _iram_text_load_addr = LOADADDR(.iram_text);
    _iram_text_start = ADDR(.iram_text);
    _iram_text_end = ADDR(.iram_text) + SIZEOF(.iram_text);
    .text :
    {
        *(.text .text.*)
        *(.icache_text .icache_text.*)
        *(.gnu.linkonce.t.*)
    } >flash
    .fini :
    {
        KEEP (*(SORT_NONE(.fini)))
    } >flash
    PROVIDE (__etext = .);
    PROVIDE (_etext = .);
    PROVIDE (etext = .);
    .rodata :
    {
        *(.rdata)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(4);
        PROVIDE( _cli_cmd_start = .);
        KEEP (*(.cli_cmd.*))
        PROVIDE( _cli_cmd_end = .);
    } >flash
    . = ALIGN(4);
    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } >flash
    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
        PROVIDE_HIDDEN (__init_array_end = .);
    } >flash
    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } >flash
    .ctors :
    {
        /* gcc uses crtbegin.o to find the start of
        the constructors, so we make sure it is
        first.  Because this is a wildcard, it
        doesn't matter if the user does not
        actually link against crtbegin.o; the
        linker won't look for a file to match a
        wildcard.  The wildcard also means that it
        doesn't matter which directory crtbegin.o
        is in.  */
        KEEP (*crtbegin.o(.ctors))
        KEEP (*crtbegin?.o(.ctors))
        /* We don't want to include the .ctor section from
        the crtend.o file until after the sorted ctors.
        The .ctor section from the crtend file contains the
        end of ctors marker and it must be last */
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*(.ctors))
    } >flash
    .dtors :
    {
        KEEP (*crtbegin.o(.dtors))
        KEEP (*crtbegin?.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*(.dtors))
    } >flash
    .data : ALIGN(8)
    {
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        PROVIDE( _gp = . + 0x800 );
        *(.srodata.cst16)
        *(.srodata.cst8)
        *(.srodata.cst4)
        *(.srodata.cst2)
        *(.srodata .srodata.*)
        *(.sdata .sdata.*)
        *(.gnu.linkonce.s.*)
        . = ALIGN(4);
    } >ram AT>flash
    PROVIDE( _data_load_addr = LOADADDR(.data) );
    PROVIDE( _data_start = ADDR(.data) );
    PROVIDE( _data_end = ADDR(.data) + SIZEOF(.data) );
    .bss : ALIGN(8)
    {
        *(.sbss*)
        *(.gnu.linkonce.sb.*)
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
    } >ram
    PROVIDE( _bss_start = ADDR(.bss) );
    PROVIDE( _bss_end = ADDR(.bss) + SIZEOF(.bss) );
    .heap :
    {
        . = ALIGN(4);
        PROVIDE( _heap_start = . );
        . = ORIGIN(ram) + LENGTH(ram) - __stack_size;
        PROVIDE( _heap_end = . );
    } >ram
    PROVIDE( _heap_size = _heap_end - _heap_start );
    .stack :
    {
        . += __stack_size;
        PROVIDE( __stack_top = . );
    } >ram
}
ASSERT(_heap_size > 0x4000, "Heap size is too small...");
PROVIDE(rom_lib_version = 0x10204774);
