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
import copy
import subprocess
import pprint

from SCons.Scanner import ClassicCPP

from wq_environment import *
from SCons.Script import *

def wq_rom_lib_src(src, incdir, env, original=[], ignore=[], empty=[]):
    nc = []
    nh = []

    for f in Flatten(src):
        if str(f).endswith('.c'):
            nc.append(env.ROMSRC(f.get_path(), str(f), IGNOREFUNC=ignore))
        else:
            nc.append(f)

    for f in Flatten(empty):
        nc.append(env.ROMSRC(f.get_path(), str(f), IGNOREFUNC=ignore, ROMSRCFLAG='--empty'))

    for p in incdir:
        env.Append(CPPPATH = Dir(p))
        h = env.Glob(os.path.join(p, '*.h'), exclude=original)
        for f in h:
            header = env.ROMSRC(os.path.join(p, f.name), str(f), IGNOREFUNC=ignore)
            env.Depends('#', header)
            nh.append(header)

    # Copy original file to dst dir
    for i in Flatten([env.Glob(o) for o in original]):
        if str(i).endswith('.h'):
            header = env.Install(i.get_dir(), str(i))
            header.append(h)
            env.Depends('#', header)

    if env.__contains__('ROMLIBPATH'):
        env.Depends(nc, os.path.join(env['ROMLIBPATH'], 'rom_lib.symbol'))
        env.Depends(nh, os.path.join(env['ROMLIBPATH'], 'rom_lib.symbol'))

    return nc

def wq_binary_info_build(target, source, env):
    cmd = env.subst("$OBJDUMP -f ${SOURCE}", source = source)
    info = subprocess.getoutput(cmd)

    start_address = ''
    for line in info.split('\n'):
        if 'start address' in line:
            start_address = line.split(' ')[-1]

    with open(str(target[0]), 'w') as f:
        f.write("entry:%s" % start_address)

def patch_func_install_src(target, source, env):
    patch_list = {
        'header': [],
        'tbl': {},
        'func': []
    }

    def add_patch_group(p):
        tbl = ''
        if p.__contains__('header') and p['header']:
            patch_list['header'].append(p['header']+ p.get('header_suffix', ''))

        if p.__contains__('tbl') and p['tbl']:
            if isinstance(p['tbl'], dict):
                tbl = list(p['tbl'].values())[0]
                patch_list['tbl'].update(p['tbl'])
            else:
                tbl = p['tbl']
                patch_list['tbl'].update({'indir_%s_t' % tbl: tbl})
        elif p.__contains__('file') and p['file']:
            tbl = p['file'] + '_api_tbl'
            if not p.__contains__('header'):
                patch_list['header'].append(p['file'] + p.get('header_suffix', ''))

        def_prefix = p.get('def_prefix', '')
        def_suffix = p.get('def_suffix', '')
        fun_prefix = p.get('fun_prefix', '')
        fun_suffix = p.get('fun_suffix', '')
        group = {
            'tbl': tbl,
            'func': {}
        }
        for fn in p.get('patch_func', []):
            if isinstance(fn, dict):
                group['func'].update(fn)
            else:
                fdef = def_prefix + fn + def_suffix
                fun = fun_prefix + fn + fun_suffix
                group['func'].update({fdef: fun})
        patch_list['func'].append(group)

    # parse all patch list json file
    for file in env['PATCHLIST']:
        with open(file, 'r') as f:
            j = json.load(f)
        if j.__contains__('module'):
            for p in j.get('patch', []):
                add_patch_group(p)
            continue

        # TODO: old format, need remove
        for m, l in j.items():
            for k, v in l.items():
                h = k if m == 'open_core' else k + '_patch'
                patch_list['header'].append(h)

                group = {
                    'tbl': k + '_api_tbl',
                    'func': {}
                }
                def_suffix = '_fn' if m == 'open_core' else ''
                fun_prefix = '' if m == 'open_core' else 'patch_'

                for f in v['patch_e'] + v['patch_i']:
                    group['func'].update({f + def_suffix: fun_prefix + f})
                patch_list['func'].append(group)


    with open(str(target[0]), 'w') as f:
        f.write(
            '/* \n'
            ' * Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.\n'
            ' * ATUO GENERATE patch function install src file.\n'
            ' * time: %s\n'
            ' */\n' % time.asctime( time.localtime(time.time()) )
        )

        # Write include
        f.write('#include "types.h"\n')
        # 'patch_funcs.h
        f.write('#include "%s"\n' % os.path.split(str(target[0]))[1].replace('.c', '.h'))

        f.write('\n')
        for h in patch_list['header']:
            f.write('#include "%s.h"\n' % h)
        f.write('\n')

        # extern tbl
        for t, b in patch_list['tbl'].items():
            f.write('extern %s %s;\n' % (t, b))
        f.write('\n')

        # Write install function
        f.write('void patch_install(void)\n{\n')
        for g in patch_list['func']:
            tbl = g['tbl']
            for d, fn in g['func'].items():
                f.write('    {tbl}.{fdef} = {func};\n'.format(tbl = tbl, fdef = d, func = fn))
            f.write('\n')
        f.write('}\n')

def rom_symbol_list(target, source, env):
    command = env.subst("$NM $SOURCE", source = source)
    out = subprocess.check_output(command, shell=True).decode('utf-8')
    with open(str(target[0]), 'w') as f:
        for l in out.splitlines():
            f.write(l.split()[2] + '\n')

def rom_linker(target, source, env):
    command = env.subst("$NM $SOURCE", source = source)
    out = subprocess.check_output(command, shell=True).decode('utf-8')
    with open(str(target[0]), 'w') as f:
        for l in out.splitlines():
            (addr, t, sym) = l.split()
            f.write('PROVIDE(%s = 0x%s);\n' % (sym, addr))

def gen_sdk_sconscruct(target, source, env):
    config = copy.copy(env['SDKCFG'])

    for key in ['built', 'env', 'type']:
        if key in config:
            del config[key]

    config['defines'].append('BUILD_FROM_SDK')
    with open(str(target[0]), 'w') as f:
        with open(str(source[0]), 'r') as sf:
            for l in sf.readlines():
                if not l.strip() == '#TARGET':
                    f.write(l)
                else:
                    f.write('target_sdk = ')
                    pprint.pprint(config, stream=f, indent=4)

def lint_src(target, source, env):
    with open(str(target[0]), 'w') as lf:
        for s in Flatten(source):
            if s.abspath.endswith('.c'):
                lf.write(s.path + '\n')

def doxygen_config_gen(target, source, env):
    for i, d in enumerate(Flatten(env['CPPDEFINES'])):
        if isinstance(d, dict):
            k, v = list(d.items())[0]
            if r'\"' in v:
                env['CPPDEFINES'][i] = {k: v.replace(r'\"', '')}
        else:
            if r'\"' in d:
                env['CPPDEFINES'][i] = d.replace(r'\"', '')

    with open(str(target[0]), 'w') as cfg:
        with open(env['DOXDEFCONFIG'], 'r') as df:
            cfg.write(df.read())

        cfg.write('OUTPUT_DIRECTORY = %s\n' % os.path.dirname(str(target[0])))
        cfg.write('PREDEFINED       = ' + ' \\\n    '.join(env.subst("${_defines('', CPPDEFINES, '', __env__)}").split()) + '\n')
        cfg.write('INPUT            = ' + ' \\\n    '.join([str(s) for s in Flatten(source)]) + '\n')
