# Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.
#
# This Information is proprietary to WuQi Technologies and MAY NOT
# be copied by any method or incorporated into another program without
# the express written consent of WuQi. This Information or any portion
# thereof remains the property of WuQi. The Information contained herein
# is believed to be accurate and WuQi assumes no responsibility or
# liability for its use in any way and conveys no license or title under
# any patent or copyright and makes no representation or warranty that this
# Information is free from patent or copyright infringement.
#
# SCons makefile

import os
import sys
import binascii
import subprocess

from SCons.Tool import SourceFileScanner
from SCons.Script import *

TOOLCHAIN_PREFIX = ['riscv64-unknown-elf-', 'riscv-none-embed-']

def bin_to_hex(target, source, env):
    with open(str(target[0]), 'w') as tf:
        with open(str(source[0]), 'rb') as sf:
            b = sf.read(4)
            while b:
                tf.write('%02x%02x%02x%02x\n' % (b[3], b[2], b[1], b[0]))
                b = sf.read(4)

bin_builder = Builder(
    action = Action("$OBJCOPY -O binary $SOURCE $TARGET", "$BINCOMSTR"),
    src_suffix = ".elf",
    suffix = ".bin"
)

tbl_builder = Builder(
    action = Action("$OBJCOPY -O binary -j .rom_tbl $SOURCE $TARGET", "$BINCOMSTR"),
    src_suffix = ".elf",
    suffix = ".tbl"
)

asm_builder = Builder(
    action = Action("$OBJDUMP -x -S $SOURCE > $TARGET", "$ASMCOMSTR"),
    src_suffix = ".elf",
    suffix = ".asm"
)

lds_builder = Builder(
    action = Action("$CC -E -C -P -x c $_CPPDEFFLAGS $_CPPINCFLAGS $SOURCE -o $TARGET", '$LDSCOMSTR'),
    suffix = ".prelds",
    source_scanner = SourceFileScanner
)

hex_builder = Builder(
    action = Action(bin_to_hex, '$HEXCOMSTR'),
    suffix = '.hex'
)

def install_riscv_tools(env):
    for perfix in TOOLCHAIN_PREFIX:
        cc = perfix + 'gcc'
        if env.WhereIs(cc) != None:
            break
    else:
        print("Con't found a valid riscv toolchain.")
        sys.exit(-1)

    env['CC'] = perfix + 'gcc'
    env['CXX'] = perfix + 'g++'
    env['AR'] = perfix + 'ar'
    env['AS'] = perfix + 'gcc'
    env['NM'] = perfix + 'nm'
    env['LINK'] = perfix + 'gcc'
    env['RANLIB'] = perfix + 'ranlib'
    env['OBJCOPY'] = perfix + 'objcopy'
    env['OBJDUMP'] = perfix + 'objdump'

    if env['PLATFORM'] == 'win32':
        env['ASCOM'] = env['ASPPCOM']

    common_flags = [
        "-march=rv32imafc",
        "-mabi=ilp32f",
        "-fno-strict-aliasing",
        "-ffunction-sections",
        "-fdata-sections",
        "-fno-tree-switch-conversion",
        "-fno-jump-tables",
        "-fsingle-precision-constant",
        '-fno-builtin',
        "-fno-common",
        "-nostdlib",
        "-pipe",
        "-g3",
    ]

    CCFLAGS = common_flags + [
        # Add a macro __FILE_NAME__ source file name without path
        '-D__FILE_NAME__=\\"${SOURCE.file}\\"',
        "-Wall",
        "-Werror",
        "-Wextra",
        "-Wmissing-declarations",
        "-Winit-self",
        "-Wundef",
        "-Wpointer-arith",
        "-Wlogical-op",
        "-Wjump-misses-init",
        "-Wmissing-prototypes",
        "-Wstrict-prototypes",
        "-Wno-enum-conversion",
        "-Wno-cast-function-type",
        # "-fstack-protector-all",
        "-Os",
    ]

    LINKFLAGS = common_flags + [
        "-nostartfiles",
        "-Xlinker",
        "-print-memory-usage",
        "-Os"
    ]

    # Ensure libraries are repeatedly linked, as there is a circular dependency!
    # For Whole archive library
    env[ 'WALIBFLAGS' ]= [
        '-Wl,--whole-archive',
        '${_stripixes(LIBLINKPREFIX, WALIBS, LIBLINKSUFFIX, LIBPREFIXES, LIBSUFFIXES, __env__)}',
        '-Wl,--no-whole-archive'
    ]

    env['LINKCOM']= '$LINK -o $TARGET $LINKFLAGS $__RPATH $SOURCES $_LIBDIRFLAGS -Wl,--embedded-relocs -Wl,--start-group $_LIBFLAGS $WALIBFLAGS -Wl,--end-group'

    env.Append(CCFLAGS = CCFLAGS)
    env.Append(ASFLAGS = CCFLAGS)
    env.Append(LINKFLAGS = LINKFLAGS)

    # By default CXXFLAGS as same as CCFLAGS so , there need replace it.
    env.Replace(CXXFLAGS = CCFLAGS)

    env.Append(BUILDERS = {"BIN": bin_builder})
    env.Append(BUILDERS = {"TBL": tbl_builder})
    env.Append(BUILDERS = {"ASM": asm_builder})
    env.Append(BUILDERS = {'LDS': lds_builder})
    env.Append(BUILDERS = {'HEX': hex_builder})

    if int(ARGUMENTS.get('VERBOSE', '0')) == 0:
        env['CCCOMSTR'] = 'Compiling $SOURCE ...'
        env['ASCOMSTR'] = 'Compiling $SOURCE ...'
        env['ASPPCOMSTR'] = 'Compiling $SOURCE ...'
        env['CXXCOMSTR'] = 'Compiling $SOURCE ...'
        env['RANLIBCOMSTR'] = 'Ranlib $TARGET ...'
        env['ARCOMSTR'] = 'Archiving $TARGET ...'
        env['LINKCOMSTR'] = 'Linking $TARGET ...'
        env['INSTALLSTR'] = 'Install $SOURCE ...'
        env['BINCOMSTR'] = "Generating $TARGET ..."
        env['ASMCOMSTR'] = "Generating $TARGET ..."
        env['LDSCOMSTR'] = "Generating $TARGET ..."
        env['HEXCOMSTR'] = "Generating $TARGET ..."
        env['DBGCOMSTR'] = "Generating $TARGET ..."
