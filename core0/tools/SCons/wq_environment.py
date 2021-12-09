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
import subprocess

from multiprocessing import Lock

import riscv_tools
import wq_tools

from SCons.Script import *
from SCons.Errors import *

CURRDIR = os.path.split(os.path.realpath(__file__))[0]
TOPDIR = os.path.dirname(os.path.dirname(CURRDIR))

class WQTargetConfig():
    def __init__(self, config = {}):
        self._lock = Lock()
        self._file_list = {}
        self._sn = 0

    def set_config(self, config, env):
        self.config = config
        self.env = env
        self.target_chip = config.get('chip', 'wq7033')
        self.target_project = config.get('project', '')
        self.target_app = config.get('app', '')
        self.target_name = config.get('name', '')
        self.target_core = config.get('core', '')
        self.target_libs = config.get('libs', [])
        self.target_walibs = config.get('walibs', [])
        self.target_memory = config.get('memory', '')
        self.target_platform = config.get('platform', 'chip')
        self.use_os = config.get('os', '')
        self.romlibs = config.get('romlibs', '')
        self.romver = config.get('rom_version', '')
        self.build_type = config.get('type', 'bin')
        self.defines = config.get('defines', [])
        self.options = config.get('options', {})
        self.output = config.get('output', '')
        self.sub_target = config.get('sub_target', [])
        self.customer = config.get('customer', '')
        self.need_repack = False
        self.product = ['elf', 'bin', 'mem', 'asm']

        self.env['TARGET_SN'] = self.sn

        self._set_env()
        self._set_config()

    def _set_env(self):
        self.env['GEN_FID'] = self.get_file_id
        self.env['FILE_ID'] = '${GEN_FID(SOURCE, __env__)}'

        self.env.Append(CPPDEFINES = self.defines)
        self.env.Append(CPPDEFINES = 'BUILD_CORE_%s' % self.target_core.upper())
        self.env.Append(CPPDEFINES = 'BUILD_CHIP_%s' % self.target_chip.upper())
        #TODO: just for build, need remove!!!
        self.env.Append(CPPDEFINES = 'BUILD_CHIP_%s' % self.target_chip[2:].upper())
        self.env.Append(CCFLAGS = '-D__FILE_ID=$FILE_ID')

        if self.target_project in ['pbl','sbl']:
            self.env.Append(CCFLAGS = ['-fomit-frame-pointer', '-msave-restore'])
        else:
            self.env.Append(CCFLAGS = '-fno-omit-frame-pointer')

        if self.build_type in ['bin', 'rom_lib', 'patch']:
            self.env.Append(LINKFLAGS = "-Wl,-Map=%s.map" % os.path.join(self.get_build_dir(), self.get_target_name()))
        elif self.build_type == 'lint':
            self.env.Append(LNTPATH = [os.path.join(TOPDIR, 'tools', 'lint')])
            self.env.Append(LNTFILE = ['std.lnt'])

        if self.build_type == 'bin':
            self.env.Append(LINKFLAGS = "-Wl,-gc-sections")
        elif self.build_type == 'rom_lib':
            self.env.Append(PATCHSRCFLAG = '--tbl-path=' + os.path.join(self.get_build_dir(), 'inc'))
        elif self.build_type == 'patch':
            if self.romver == '0.0':
                rom_lib_path = os.path.join(Dir('#').path, 'output', 'rom_lib', self.target_core)
            else:
                rom_lib_path = os.path.join(Dir('#').path, 'prebuild', 'rom_lib', self.romver, self.target_core)
                if not os.path.exists(rom_lib_path):
                    raise UserError("Not open rom lib path %s!" % rom_lib_path)

            rom_lib = os.path.join(rom_lib_path, 'rom_lib.elf')
            self.env.Append(PATCHSRCFLAG = '-P')
            self.env.Append(ROMLIBPATH = rom_lib_path)
            self.env.Append(CCFLAGS = ['-Wno-unused-variable', '-Wno-unused-function'])
            self.env.Append(LINKFLAGS = "-Wl,-gc-sections")
            self.env.Append(CPPPATH = os.path.split(rom_lib)[0])
            self.env.Append(CPPDEFINES = 'BUILD_PATCH')

            self.env.Depends('#', rom_lib)

    def _set_config(self):
        for p in self.config.get('product', []):
            if not p in self.product:
                self.product.append(p)

        # Add command line macro
        for d in ARGUMENTS.get('def', '').split(','):
            if d.strip():
                print ('Add define %s' % d)
                self.env.Append(CPPDEFINES = d)

        pkg = ARGUMENTS.get('repack', '')
        if pkg and os.path.exists(pkg):
            self.need_repack = True
            self.prebuilt_wpk = File(pkg)

        # Set base file id
        if self.target_core in ['core0']:
            file_id_base = 0
        elif self.target_core in ['core1']:
            file_id_base = 1000
        else:
            raise ValueError('target name illegal')

        if self.build_type in ['rom_lib']:
            file_id_base += 2000

        self.env.Append(FILEIDBASE = file_id_base)

    @property
    def sn(self):
        _ = self._sn
        self._sn += 1
        return _

    def get_file_list(self, env):
        return self._file_list[env['TARGET_SN']]

    def get_file_id(self, source, env):
        with self._lock:
            sn = env['TARGET_SN']
            if not sn in self._file_list:
                self._file_list[sn] = []
            _file_list = self._file_list[sn]
            if not source.path in _file_list:
                _file_list.append(source.path)
            return env['FILEIDBASE'] + _file_list.index(source.path)

    def add_stdinc(self, env):
        def get_gcc_include_path(env):
            inc_paths = [
                "include",
                "include-fixed",
                "../../../../%s/include" % (env['CC'][:-4])
            ]

            command = env['CC'] + ' --print-libgcc-file-name'
            libname = subprocess.check_output(command, shell=True).decode('utf-8')
            basepath = os.path.dirname(os.path.dirname(os.path.dirname(libname)))
            return ['/'.join([basepath, d]) for d in inc_paths]
        env.Append(CPPPATH = get_gcc_include_path(env))

    def get_target_project(self):
        return self.target_project if self.target_project else 'unkonwn'

    def get_target_app(self):
        return self.target_app

    def get_target_name(self):
        return self.target_name if self.target_name else '_'.join([self.target_project, self.target_core])

    def get_target_core(self):
        return self.target_core

    def get_target_libs(self):
        return self.target_libs

    def get_target_walibs(self):
        return self.target_walibs

    def get_target_memory(self):
        return self.target_memory

    def get_target_driver(self):
        return self.target_driver

    def get_target_platform(self):
        return self.target_platform

    def get_use_os(self):
        return self.use_os

    def get_romlibs(self):
        return self.romlibs

    def get_build_dir(self):
        output_project = self.output if self.output else self.target_project
        output_project = output_project + '_' + self.target_chip if self.target_chip != 'wq7033' else output_project
        if self.build_type in ['bin']:
            name = output_project if not self.customer else '-'.join([output_project, self.customer])
            return os.path.join('output', self.build_type, name, self.get_target_core())
        elif self.build_type == 'patch':
            return os.path.join('output', self.build_type, output_project, self.romver, self.get_target_core())
        elif self.build_type in ['rom_lib']:
            name = self.build_type + '_' + self.target_chip if self.target_chip != 'wq7033' else self.build_type
            return os.path.join('output', name, self.get_target_core())
        elif self.build_type in ['sdk', 'doxygen']:
            return os.path.join('output', self.build_type, self.get_target_core())
        else:
            return os.path.join('output', self.build_type, output_project, self.get_target_core())

    def get_target_product(self):
        return self.product

    def get_target_options(self):
        return self.options

    def get_build_type(self):
        return self.build_type

    def get_rom_version(self):
        return self.romver

    def get_sub_target(self):
        return self.sub_target

    def get_customer(self):
        return self.customer

    def get_target_chip(self):
        return self.target_chip

class WQPackageConfig():
    def __init__(self, config = {}):
        pass

    def set_config(self, config):
        self.name = config.get('name', 'example')
        self.output = config.get('output', '')
        self.build_type = config.get('type', 'package')
        self.prebuild = config.get('prebuild', [])
        self.chip = config.get('chip', 'wq7033')

    def get_package_name(self):
        return self.name

    def get_package_chip(self):
        return self.chip

    def get_output_path(self):
        output = self.output if self.output else self.name
        return os.path.join('output', self.build_type, output)

    def get_prebuild(self):
        return self.prebuild

class WQEnvironment(Environment):
    def __init__(self):
        super(WQEnvironment, self).__init__(tools=['gcc', 'ar', 'gas', 'g++', 'gnulink'], ENV = os.environ)
        self.install_wq_env()

        if self._dict['PLATFORM'] == 'win32':
            self.install_spawn()
            self._dict['PYCOM'] = 'python'
        else:
            self._dict['PYCOM'] = 'python3'

    def install_spawn(self):
        old_spawn = self._dict['SPAWN']

        def my_spawn(sh, escape, cmd, args, spawnenv):
            # If we're trying to use redirection, just use the original one and hope there isn't also
            # a long command line.
            if ">" in args:
                return old_spawn(sh, escape, cmd, args, spawnenv)

            newargs = ' '.join(args[1:])
            cmdline = cmd + " " + newargs
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            proc = subprocess.Popen(cmdline, startupinfo=startupinfo, shell = False, env = spawnenv)
            rv = proc.wait()
            return rv

        self._dict['SPAWN'] = my_spawn

    def add_link_script(self):
        self.Append(LIBPATH = ['.'])
        self.Append(LINKFLAGS = "-Tlink.lds")

    def install_wq_env(self):
        riscv_tools.install_riscv_tools(self)
        wq_tools.install_wq_tools(self)
        self.add_link_script()

WQTargetConfig = WQTargetConfig()
WQPackageConfig = WQPackageConfig()
