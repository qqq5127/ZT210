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
import json
import subprocess

from SCons.Script import *
from wq_actions import *
from dbglog_tool import *

import wq_environment

def wq_binary_info_build(target, source, env):
    cmd = env.subst("$OBJDUMP -f ${SOURCE}", source = source)
    info = subprocess.getoutput(cmd)

    start_address = ''
    for line in info.split('\n'):
        if 'start address' in line:
            start_address = line.split(' ')[-1]

    with open(str(target[0]), 'w') as f:
        f.write("start:%s" % start_address)

ah_builder = Builder(
    action = Action("$PYCOM $ADD_HEADER -c $FW_CFG -I $BINARY_PATH $SOURCES -o $TARGET", '$AHCOMSTR'),
    suffix = ".bin"
)

lzma_builder = Builder(
    action = Action("$ELZMA e $SOURCES $TARGET", '$LZMACOMSTR'),
    suffix = ".lzma.bin"
)

info_builder = Builder(
    action = Action(wq_binary_info_build, '$GENERATESTR'),
    suffix = ".info"
)

mem_builder = Builder(
    action = Action("$PYCOM $MEM_STAT $SOURCES > $TARGET", '$GENERATESTR'),
    suffix = ".mem"
)

def romsrc_scanner(node, env, path):
    if (env.__contains__('PATCHLIST')):
        return(env['PATCHLIST'])
    return []

rom_src_builder = Builder(
    action = Action("$PYCOM $ROM_SRC_TOOL $SOURCES -o $TARGET $_IGNOREFUNC $ROMSRCFLAG $PATCHSRCFLAG", '$GENERATESTR'),
    source_scanner = Scanner(romsrc_scanner)
)

rom_symbol_builder = Builder(
    action = Action(rom_symbol_list, "$GENERATESTR"),
    suffix = ".symbol"
)

rom_linker_builder = Builder(
    action = Action(rom_linker, "$GENERATESTR"),
    suffix = ".lds"
)

rom_memory_builder = Builder(
    action = Action("$PYCOM $ROM_MEM_TOOL $SOURCES > $TARGET", "$GENERATESTR"),
    suffix = ".h"
)

patch_tbl_builder = Builder(
    action = Action("$PYCOM $PATCHTBL_TOOL $_PATCHLIST -r $ROMLIBPATH -o $TARGET $SOURCE", "$GENERATESTR"),
    suffix = ".tbl"
)

patch_list_builder = Builder(
    action = Action("$PYCOM $PATCHLIST_TOOL -r $TBLPATH -o $TARGET", "$GENERATESTR"),
    suffix = ".json"
)

patch_symbol_builder = Builder(
    action = Action("$PYCOM $PATCHLIST_TOOL -e -o $TARGET $SOURCES", "$GENERATESTR"),
    suffix = ".lds"
)

lint_src_builder = Builder(
    action = Action(lint_src, "$GENERATESTR"),
    suffix = '.lnt',
)

lint_builder = Builder(
    action = Action(r'$PCLINT "-os(${TARGET.path})" $_LNTPATH -zero "-summary(${TARGET.dir}/${TARGET.filebase}.summary)" $LNTFILE $_CPPDEFFLAGS $_CPPINCFLAGS $SOURCES', '$LINTSTR'),
    suffix = '.lint',
)

lint_check_builder = Builder(
    action = Action(r'$PYCOM $LINTRST_TOOL $TARGET $SOURCES $LINTDIR', "$GENERATESTR"),
    suffix = '.txt',
)

rpc_builder = Builder(
    action = Action(r'$PYCOM $RPC_TOOL $RPC_CONFIG $DOMAIN $OUT_PATH', '$GENERATESTR'),
    src_suffix = '.json',
    suffix = '.h'
)

dbglog_table_builder = Builder(
    action = Action(generate_dbglog_table, "$GENERATESTR"),
    suffix = '.log'
)

def fid_emitter(target, source, env):
    if source[0].is_derived():
        wq_environment.WQTargetConfig.get_file_id(source[0], env)
    else:
        wq_environment.WQTargetConfig.get_file_id(source[0].srcnode(), env)

    return target, source

def install_wq_tools(env):
    env['BUILDERS']['Object'].add_emitter('.c', fid_emitter)

    env.Append(ADD_HEADER = os.path.join(wq_environment.CURRDIR, '..', 'add_header', 'add_header.py'))
    env.Append(MEM_STAT = os.path.join(wq_environment.CURRDIR, '..', 'scripts', 'mem_stat.py'))
    env.Append(ELZMA = os.path.join(wq_environment.CURRDIR, '..', 'elzma', 'elzma'))
    env.Append(ROM_SRC_TOOL = os.path.join(wq_environment.CURRDIR, '..', 'rom_lib', 'rom_lib_src.py'))
    env.Append(ROM_MEM_TOOL = os.path.join(wq_environment.CURRDIR, '..', 'rom_lib', 'rom_lib_memory.py'))
    env.Append(PATCHTBL_TOOL = os.path.join(wq_environment.CURRDIR, '..', 'patch', 'patch_tbl.py'))
    env.Append(PATCHLIST_TOOL = os.path.join(wq_environment.CURRDIR, '..', 'patch', 'patch_list.py'))
    env.Append(LINTRST_TOOL= os.path.join(wq_environment.CURRDIR, '..', 'lint', 'lint_result.py'))
    env.Append(RPC_TOOL = os.path.join(wq_environment.CURRDIR, 'rpc_autogen.py'))
    env.Append(DOXDEFCONFIG = os.path.join(wq_environment.CURRDIR, '..', 'doxygen', 'config.txt'))
    env.Append(PCLINT = 'lint-nt.exe')

    env.Append(BUILDERS = {'AH': ah_builder})
    env.Append(BUILDERS = {'LZMA': lzma_builder})
    env.Append(BUILDERS = {'INFO': info_builder})
    env.Append(BUILDERS = {'MEM': mem_builder})
    env.Append(BUILDERS = {'ROMSRC': rom_src_builder})
    env.Append(BUILDERS = {'ROMSYMBOL': rom_symbol_builder})
    env.Append(BUILDERS = {'ROMLINKER': rom_linker_builder})
    env.Append(BUILDERS = {'ROMMEM': rom_memory_builder})
    env.Append(BUILDERS = {'PATCHTBL': patch_tbl_builder})
    env.Append(BUILDERS = {'PATCHLIST': patch_list_builder})
    env.Append(BUILDERS = {'PATCHSYMBOL': patch_symbol_builder})
    env.Append(BUILDERS = {'LINTHCECK': lint_check_builder})
    env.Append(BUILDERS = {'LINTSRC': lint_src_builder})
    env.Append(BUILDERS = {'LINT': lint_builder})
    env.Append(BUILDERS = {'RPCSRC': rpc_builder})
    env.Append(BUILDERS = {'DBGTBL': dbglog_table_builder})

    env.Append(_IGNOREFUNC ='$( ${_concat("-i ", IGNOREFUNC, "", __env__)} $)')
    env.Append(_PATCHLIST ='$( ${_concat("-p ", PATCHLIST, "", __env__)} $)')
    env.Append(PATCHSRCFLAG = [])
    env.Append(LOGPKLS = [])

    env.Append(_LNTPATH ='$( ${_concat("-i", LNTPATH, "", __env__)} $)')

    if int(ARGUMENTS.get('VERBOSE', '0')) == 0:
        env['AHCOMSTR'] = "Generating Firmware Package binary ..."
        env['LZMACOMSTR'] = "LZMA binary $SOURCES ..."
        env['GENERATESTR'] = "Generating $TARGET ..."
        env['LINTSTR'] = "Linting $TARGET ..."
