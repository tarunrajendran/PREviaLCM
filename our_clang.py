#!/usr/local/bin/python3
import sys
import os

if len(sys.argv) == 1:
    print('our clang wrapper')
    sys.exit()

## print will cause lnt error. so don't print.

argv = sys.argv[1:]
#print(argv)

input_file = ""
output_file = ""

i = 0
while i < len(argv):
    if argv[i] == '-o':
        i += 1
        output_file = argv[i]
    elif argv[i][0] != '-' and (argv[i].endswith('.c') or argv[i].endswith('.cpp') or (argv[i].endswith('.o'))):
        input_file = argv[i]
    i += 1

o_file = output_file
if not (o_file.endswith('.o')):
    o_file = o_file + '.o'

#print(input_file)
#print(output_file)
#print(o_file)

if not (input_file.endswith('.o')):
    os.system('clang -c -O0 -emit-llvm ' + input_file + ' -o - | opt -stats -load /Users/wangyiyi/百度云同步盘/Github/CS426_Unit_Project/llvm/build/lib/PREviaLCM.dylib -pre | llc -filetype=obj > ' + o_file)

if not (output_file.endswith('.o')):
    os.system('clang -g ' + o_file + ' -o ' + output_file)
