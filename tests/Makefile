# -reassociate    global reaaosication
# -gvn 						global value numbering


# on my local machine (Mac)
LLVM_DIR = /Users/wangyiyi/百度云同步盘/Github/CS426_Unit_Project/llvm/build
PRE_LIB = ${LLVM_DIR}/lib/LLVMPRE.dylib

# on ews
# LLVM_DIR = /class/cs526/sp17-projects/ywang189/llvm/build
# PRE_LIB = ${LLVM_DIR}/lib/LLVMScalarRepl.so

CC = clang
CXX = clang++
OPT = ${LLVM_DIR}/bin/opt -debug -stats
LLVM_DIS = ${LLVM_DIR}/bin/llvm-dis
LLC = ${LLVM_DIR}/bin/llc

# OPTS = -load $(PRE_LIB) -inline -globaldce -instcombine -argpromotion -scalarrepl-ywang189 -sccp -instcombine -dce -simplifycfg -globaldce -verify
# OPTS = -load $(PRE_LIB) -inline -globaldce -scalarrepl-ywang189 -sccp -instcombine -dce -simplifycfg -globaldce -verify
# OPTS = -load $(PRE_LIB) -scalarrepl-ywang189
OPTS = -load $(PRE_LIB) -reassociate -gvn

# Make a .ll file from a .bc file, using llvm-dis
%.ll: %.bc
	$(LLVM_DIS) < $< > $@

# Make a .bc file from a .c file by running through llvm-gcc, with
# all optimizations turned off.
# without running ScalarRepl pass
%.bc: %.c
	$(CC) -c -O0 -emit-llvm $< -o - | $(OPT) > $@

%.s: %.bc
	$(LLC) < $< > $@

%.exe: %.s
	$(CC) -g $+ -o $@

# %.out: %.exe
#	./$< > $@ || true

clean :
	rm -f *.bc *.ll *.s *.exe *.out *.log
	rm -rf *.exe.dSYM