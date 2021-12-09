#! /usr/bin/env python3
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
import re
import pickle

from optparse import OptionParser

def scan_source_file(source):
    info_dict = {}
    with open(source, 'r', encoding='utf-8') as f:
        file_iter = enumerate(f)
        for num, line in file_iter:
            line = line.strip()
            if line.startswith('DBGLOG_'):
                if '(' not in line or line[:line.index('(')].split('_')[-1] == 'RAW':
                    # Drop raw log
                    continue

                last_num = 0
                while not re.match(r'DBGLOG_.*\(.*".+".*\);', line):
                    last_num, new_line = file_iter.__next__()
                    line += new_line.strip()

                m = re.match(r'DBGLOG_(.*)\(\s*".+"(.*)\);', line)
                if m :
                    # For gcc 7.2 the line number is the end line of log macro
                    info_dict.setdefault(num + 1, m.group())
                    # For gcc 10.2 the line number is the start line of log macro
                    last_num and info_dict.setdefault(last_num + 1, m.group())
    return info_dict

def generate_dbglog_table(target, source, env):
    items = []
    # read prebuilt pkl tables
    for pkl in env.Flatten(env['LOGPKLS']):
        if os.path.exists(str(pkl)) and os.path.getsize(str(pkl)):
            with open(str(pkl), 'rb') as f:
                items.extend(pickle.load(f))

    for index, f in enumerate(env.Flatten(source)):
        item = [env['GEN_FID'](f, env), f.path, scan_source_file(f.path)]
        items.append(item)

    items.sort(key = lambda x: x[0])

    with open(target[0].path, 'wb') as pkl:
        pickle.dump(items, pkl)

def get_file_list(log):
    with open(log, 'rb') as f:
        data_list = pickle.load(f)
        return [fn for i, fn, logs in data_list]

def merge_txt_table(target, source, env):
    data_list = []
    def remove(n):
        for d in data_list:
            if d[0] == n:
                data_list.remove(d)

    for s in source:
        with open(str(s), 'r') as f:
            for l in f.readlines():
                try:
                    lf = eval(l)
                    if lf[0] in list(map(lambda x:x[0], data_list)):
                        remove(lf[0])
                    data_list.append(lf)
                except Exception as e:
                    pass

    data_list.sort(key=lambda x:x[0])
    with open(str(target[0]),'w') as f:
        f.write('\n'.join(list(map(str, data_list))))

def gen_txt_table(target, source, env):
    data_list = []
    for i in source:
        with open(str(i), 'rb') as file_in:
            data_list += pickle.load(file_in)
    with open(str(target[0]),'w') as file_out:
        for item in data_list:
            print(item, file = file_out)

def pretty_dump_pkl(out_put, in_put):
    with open(in_put, 'rb') as file_in:
        data_list = pickle.load(file_in)
    with open(out_put,'w') as file_out:
        for item in data_list:
            print(item, file = file_out)

def main(args):
    parser = OptionParser()

    parser.add_option("-o", "--output", dest="output", default = './default.table', help="Output file path.")
    parser.add_option("-i", "--input", dest="input", help="Input pkl file path.")
    parser.add_option("-s", "--scan", dest="scan", action="store_true", help="Scan source file.")

    (options, args) = parser.parse_args(args)

    if options.scan:
        for f in args:
            print(scan_source_file(f))
        return

    pretty_dump_pkl(options.output, options.input)

if __name__ == "__main__":
    main(sys.argv[1:])
