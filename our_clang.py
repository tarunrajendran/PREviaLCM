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
    elif argv[i][0] != '-' and (argv[i].endswith('.c') or argv[i].endswith('.o')):
        input_file = argv[i]
    i += 1


o_file = output_file
if not (o_file.endswith('.o')):
    o_file = o_file + '.o'

#print(input_file)
#print(output_file)
#print(o_file)

# .c -> .o
if (input_file.endswith('.c')):
    os.system('clang -c -O0 -emit-llvm ' + input_file + ' -o - | /Users/tarunraj/Projects/build/bin/opt -stats -load /Users/tarunraj/Projects/build/lib/PREviaLCM.dylib -reassociate -newgvn | /Users/tarunraj/Projects/build/bin/llc -filetype=obj > ' + output_file)
    if not (output_file.endswith('.o')):
        os.system('clang ' + o_file + ' -o ' + output_file)
elif (len(output_file) > 0):  # .o -> exe
    os.system('clang ' + input_file + ' -o ' + output_file)
else: # else
    os.system('clang ' + argv)
