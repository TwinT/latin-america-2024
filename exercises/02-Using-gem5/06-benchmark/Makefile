GXX = riscv64-linux-gnu-g++
GEM5_PATH ?= /workspaces/latin-america-2024/gem5

matrix-multiply: matrix-multiply.c
	$(GXX) -o matrix-multiply matrix-multiply.c \
	-I$(GEM5_PATH)/include \
	-L$(GEM5_PATH)/util/m5/build/riscv/out \
	-lm5 -static

clean:
	rm -f matrix-multiply
