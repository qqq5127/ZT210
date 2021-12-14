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

from wq_tools import *
from wq_environment import *

from SCons.Script import *


class WQModule():
    def __init__(self, name, src, env, **kargs):
        self.name = name
        self.g_env = env
        self.kw = kargs
        self.base = Dir('.')
        self.src = [File(s) for s in Flatten(src)]

        allowed_keys = ['pub_inc', 'pri_inc', 'defines', 'exclude', 'resources', 'flags', 'libs', 'sub_mod']
        self.__dict__.update((key, []) for key in allowed_keys)
        self.__dict__.update((k, env.Flatten([v])) for k, v in kargs.items() if k in allowed_keys)

        self.g_env.Append(CPPPATH=self.pub_inc)

        if self.exclude:
            self.g_env.Append(EXCLUDE = [os.path.join(self.base.abspath, e) for e in self.exclude])

    def add_sub_mod(self, mod):
        self.sub_mod.append(mod)

    def get_all_sub_folder(self, sources):
        d = [self.base.srcnode().abspath]
        for s in sources:
            if not os.path.dirname(s.srcnode().abspath) in d:
                d.append(os.path.dirname(s.srcnode().abspath))

        return d

    def library(self):
        libs = []

        if self.libs:
            for l in self.libs:
                if str(l).endswith(self.g_env['LIBSUFFIX']):
                    # With .a is a full name lib
                    libs.append(self.g_env.File(os.path.join(self.base.srcnode().abspath, str(l))))
                elif os.sep in str(l):
                    # is a path and not ends with .a
                    # eg: 'src/module_name/lib/name', 'should be src/module_name/lib/libname.a'
                    path, name = os.path.split(str(l))
                    libname = self.g_env.subst("${LIBPREFIX}%s${LIBSUFFIX}" % name)
                    libs.append(self.g_env.File(os.path.join(self.base.srcnode().abspath, path, libname)))
                else:
                    # just a name eg: 'gcc'
                    libname = self.g_env.subst("${LIBPREFIX}%s${LIBSUFFIX}" % str(l))
                    libs.append(self.g_env.File(os.path.join(self.base.srcnode().abspath, libname)))

        env = self.g_env.Clone()
        env.Append(CPPPATH=self.pri_inc)
        env.Append(CPPDEFINES=self.defines)
        env.Append(CCFLAGS=self.flags)

        for s in self.sub_mod:
            self.src.extend(s.objects())

        output = os.path.join(str(self.base), self.name)
        libs.extend(env.Library(output, self.src))
        return libs

    def build(self):
        build_type = WQTargetConfig.get_build_type()

        if build_type in ['bin', 'rom_lib', 'patch', 'sdk']:
            return self.library()

        elif build_type in ['lint']:
            self.g_env.Append(CPPPATH=self.pri_inc)
            return self.src
        elif build_type in ['doxygen']:
            doc_files = []
            for s in self.sub_mod:
                doc_files.extend(s.build())

            if self.name in WQTargetConfig.config['sdk_prebuild']:
                # public header
                for i in Flatten(self.pub_inc):
                    doc_files.extend(Glob(os.path.join(i, '*.h')))

            else:
                # All src and header
                for i in Flatten(self.pub_inc + self.pri_inc):
                    doc_files.extend(Glob(os.path.join(i, '*.h')))
                doc_files.extend(self.src)

            return doc_files

    def gen_sdk(self):
        env = self.g_env.Clone()

        # For sub module
        for s in self.sub_mod:
            s.gen_sdk()

        sdk_sources = []

        if self.name in WQTargetConfig.config['sdk_prebuild']:
            # prebuild library
            lib = self.library()[0]
            env.Install(os.path.dirname(lib.srcnode().path), lib)
        else:
            # source file
            for i in self.src:
                if not i in sdk_sources:
                    sdk_sources.append(i)

            # Private header file
            for d in self.pri_inc:
                for i in Glob(os.path.join(d, '*.h'), exclude=self.exclude):
                    if not i in sdk_sources:
                        sdk_sources.append(i)

        # sdk special file
        for d in self.get_all_sub_folder(sdk_sources):
            for f in Glob(os.path.join(d, '*.sdk')):
                t = File(f.srcnode().abspath[:-len('.sdk')])
                if File(f.path[:-len(".sdk")]) in env.FindInstalledFiles():
                    continue
                if t in sdk_sources:
                    sdk_sources.remove(t)
                env.InstallAs(t.srcnode().path, f)

        for s in sdk_sources:
            env.Install(os.path.dirname(s.srcnode().path), s)

        # Resources
        for r in self.resources:
            print (str(r))
            env.Install(self.base, r)

    def objects(self):
        build_type = WQTargetConfig.get_build_type()

        env = self.g_env.Clone()
        env.Append(CPPPATH=self.pri_inc)
        env.Append(CPPDEFINES=self.defines)
        env.Append(CCFLAGS=self.flags)

        objs = []
        # print (self.base.path)
        if build_type in ['bin', 'rom_lib', 'patch']:
            for s in Flatten(self.src):
                objs.extend(env.Object(s))
            return objs
        elif build_type in ['lint']:
            return self.src

def build_library(mods):
    libs = []
    for m in Flatten(mods):
        if isinstance(m, WQModule):
            libs.append(m.build())
        else:
            libs.append(m)
    return Flatten(libs)
