# Copyright(c) 2021 by WuQi Technologies. ALL RIGHTS RESERVED.
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
import datetime
import subprocess
import zipfile

from SCons.Script import *
from dbglog_tool import gen_txt_table, merge_txt_table

import wq_environment

def zip_package(target, source, env):
    with zipfile.ZipFile(str(target[0]), 'w', zipfile.ZIP_DEFLATED) as zf:
        with zipfile.ZipFile(str(env['PKG']), 'r', zipfile.ZIP_DEFLATED) as pkg:
            for f in pkg.namelist():
                if not f in [s.name for s in Flatten(source)]:
                    zf.writestr(f, pkg.read(f))
        for s in Flatten(source):
            if s.name in pkg.namelist():
                zf.write(str(s), s.name)

def extract_log(target, source, env):
    with open(str(target[0]), 'w') as f:
        f.write(zipfile.ZipFile(str(Flatten(source)[0]), 'r', zipfile.ZIP_DEFLATED).read('dbglog_table.txt').decode('utf-8'))

def merge_log(pkg, logs, env):
    name = wq_environment.WQTargetConfig.get_target_name()
    nlog = env.Command(name + '.txt', logs, Action(gen_txt_table, "$GENERATESTR"))

    log_table = env.Command('log_table.txt', pkg, Action(extract_log, "Extract $TARGET ..."))

    return env.Command('dbglog_table.txt', [log_table, nlog], Action(merge_txt_table, "$GENERATESTR"))

def update_memory_config(target, source, env):
    mem_cfg = zipfile.ZipFile(str(env['PKG']), 'r', zipfile.ZIP_DEFLATED).read('memory_config.json').decode('utf-8')
    mem_cfg = json.loads(mem_cfg)
    with open(str(Flatten(source)[0]), 'r') as f:
        key, value = tuple(f.read().strip().split(':'))
        for image in mem_cfg['images']:
            if image['id'] == '0x20' and key == 'start':
                image['start'] = eval(value)

    with open(str(target[0]), 'w') as f:
        json.dump(mem_cfg, f, indent=4)

def repack(pkg, firmwares, env, cus=''):
    logs = []
    for f in Flatten(firmwares):
        if str(f).endswith('.log'):
            logs.append(str(f))

    log = merge_log(pkg, logs, env)

    for f in Flatten(firmwares):
        if str(f).endswith('.info'):
            mem = env.Command('memory_config.json', f, Action(update_memory_config, "$GENERATESTR"), PKG=pkg)

    for f in Flatten(firmwares):
        if str(f).endswith('.bin'):
            bin = f

    cus_prebuild = Glob(os.path.join('#', 'src', 'customer', cus, 'prebuild', '*.*'))
    zip_version = ARGUMENTS.get('BUILD_VERSION', '0.0.0.0')

    if cus:
        zip_name = '-'.join([pkg.name.split('-')[0], cus, zip_version + '.wpk'])
    else:
        zip_name = '-'.join([pkg.name.split('-')[0], zip_version + '.wpk'])

    zip_path = os.path.join('#', 'output', 'package', zip_name)
    zip_action = Action(zip_package, '$GENERATESTR')
    env.Command(zip_path, [log, mem, bin, cus_prebuild], zip_action, PKG=pkg)
