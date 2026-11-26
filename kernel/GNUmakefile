
# Nuke built-in rules.
.SUFFIXES:

# This is the name that our final executable will have.
override OUTPUT := kernel

# Target architecture to build for. Default to x86_64.
ARCH := x86_64

# Install prefix; /usr/local is a good, standard default pick.
PREFIX := /usr/local

# Check if the architecture is supported.
ifeq ($(filter $(ARCH),aarch64 loongarch64 riscv64 x86_64),)
    $(error Architecture $(ARCH) not supported)
endif

# User controllable toolchain and toolchain prefix.
TOOLCHAIN :=
TOOLCHAIN_PREFIX :=
ifneq ($(TOOLCHAIN),)
    ifeq ($(TOOLCHAIN_PREFIX),)
        TOOLCHAIN_PREFIX := $(TOOLCHAIN)-
    endif
endif

# User controllable C compiler command.
ifneq ($(TOOLCHAIN_PREFIX),)
    CC := $(TOOLCHAIN_PREFIX)gcc
else
    CC := cc
endif

# User controllable linker command.
LD := $(TOOLCHAIN_PREFIX)ld

# User controllable Rust compiler.
RUSTC := rustc

# Defaults overrides for variables if using "llvm" as toolchain.
ifeq ($(TOOLCHAIN),llvm)
    CC := clang
    LD := ld.lld
endif

# User controllable C flags.
CFLAGS := -g -O2 -pipe

# User controllable C preprocessor flags.
CPPFLAGS :=

# User controllable Rust flags.
RUSTFLAGS :=

# Rust edition.
RUST_EDITION := 2021

# Rust target. Override this if using a custom target.
ifeq ($(ARCH),x86_64)
    RUST_TARGET := x86_64-unknown-none
endif
ifeq ($(ARCH),aarch64)
    RUST_TARGET := aarch64-unknown-none
endif
ifeq ($(ARCH),riscv64)
    RUST_TARGET := riscv64gc-unknown-none-elf
endif
ifeq ($(ARCH),loongarch64)
    RUST_TARGET := loongarch64-unknown-none
endif

# Rust crate root.
RUST_CRATE_ROOT := src/rust/lib.rs

# Additional Rust libraries to link.
RUST_LIBS :=

ifeq ($(ARCH),x86_64)
    # User controllable nasm flags.
    NASMFLAGS := -g
endif

# User controllable linker flags.
LDFLAGS :=

# Ensure the dependencies have been obtained.
ifneq ($(filter-out clean distclean,$(or $(MAKECMDGOALS),default)),)
    ifeq ($(wildcard .deps-obtained),)
        $(error Please run the ./get-deps script first)
    endif
endif

# Check if CC is Clang.
override CC_IS_CLANG := $(shell ! $(CC) --version 2>/dev/null | grep -q '^Target: '; echo $$?)

# Internal C flags that should not be changed by the user.
override CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -nostdinc \
    -ffreestanding \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-lto \
    -fno-PIC \
    -ffunction-sections \
    -fdata-sections \
    -Wno-error=implicit-function-declaration

# Internal C preprocessor flags that should not be changed by the user.
override CPPFLAGS := \
    -I src \
    -I limine-protocol/include \
    -isystem freestanding-c-hdrs/include \
    $(CPPFLAGS) \
    -MMD \
    -MP

ifeq ($(ARCH),x86_64)
    # Internal nasm flags.
    override NASMFLAGS := \
        $(patsubst -g,-g -F dwarf,$(NASMFLAGS)) \
        -Wall
endif

# Architecture specific internal flags.
ifeq ($(ARCH),x86_64)
    ifeq ($(CC_IS_CLANG),1)
        override CC += \
            -target x86_64-unknown-none-elf
    endif

    override CFLAGS += \
        -m64 \
        -march=x86-64 \
        -mabi=sysv \
        -mno-80387 \
        -mno-mmx \
        -mno-sse \
        -mno-sse2 \
        -mno-red-zone \
        -mcmodel=kernel

    override LDFLAGS += \
        -m elf_x86_64

    override NASMFLAGS := \
        -f elf64 \
        $(NASMFLAGS)
endif

ifeq ($(ARCH),aarch64)
    ifeq ($(CC_IS_CLANG),1)
        override CC += \
            -target aarch64-unknown-none-elf
    endif

    override CFLAGS += \
        -mcpu=generic \
        -march=armv8-a+nofp+nosimd \
        -mgeneral-regs-only

    override LDFLAGS += \
        -m aarch64elf
endif

ifeq ($(ARCH),riscv64)
    ifeq ($(CC_IS_CLANG),1)
        override CC += \
            -target riscv64-unknown-none-elf
    endif

    override CFLAGS += \
        -march=rv64imac_zicsr_zifencei \
        -mabi=lp64 \
        -mno-relax

    override LDFLAGS += \
        -m elf64lriscv \
        --no-relax
endif

ifeq ($(ARCH),loongarch64)
    ifeq ($(CC_IS_CLANG),1)
        override CC += \
            -target loongarch64-unknown-none-elf
    endif

    override CFLAGS += \
        -march=loongarch64 \
        -mabi=lp64s \
        -mfpu=none \
        -msimd=none

    override LDFLAGS += \
        -m elf64loongarch
endif

# Internal linker flags that should not be changed by the user.
override LDFLAGS += \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    --gc-sections \
    -T linker-scripts/$(ARCH).lds

# Find all source files.
override SRCFILES := $(shell find -L src cc-runtime/src -type f \
    -not -path 'src/arch/*' 2>/dev/null | LC_ALL=C sort)

# Add architecture specific files, if they exist.
override SRCFILES += $(shell find -L src/arch/$(ARCH) -type f \
    2>/dev/null | LC_ALL=C sort)

# Obtain source file lists.
override CFILES := $(filter %.c,$(SRCFILES))
override ASFILES := $(filter %.S,$(SRCFILES))

ifeq ($(ARCH),x86_64)
    override NASMFILES := $(filter %.asm,$(SRCFILES))
endif

# Obtain object file names.
override OBJ := \
    $(addprefix obj-$(ARCH)/,$(CFILES:.c=.c.o)) \
    $(addprefix obj-$(ARCH)/,$(ASFILES:.S=.S.o))

ifeq ($(ARCH),x86_64)
    override OBJ += \
        $(addprefix obj-$(ARCH)/,$(NASMFILES:.asm=.asm.o))
endif

# Find Rust source files.
override RUST_SOURCES := $(shell find -L src/rust -type f -name '*.rs' \
    2>/dev/null | LC_ALL=C sort)

# Rust static library.
override RUST_LIB :=
ifneq ($(wildcard $(RUST_CRATE_ROOT)),)
    override RUST_LIB := obj-$(ARCH)/librustkernel.a
endif

# Obtain C and assembly dependency file names.
override HEADER_DEPS := \
    $(addprefix obj-$(ARCH)/,$(CFILES:.c=.c.d)) \
    $(addprefix obj-$(ARCH)/,$(ASFILES:.S=.S.d))

# Default target. This must come first, before header dependencies.
.PHONY: all
all: bin-$(ARCH)/$(OUTPUT)

# Include header dependencies.
-include $(HEADER_DEPS)

# Link rules for the final executable.
bin-$(ARCH)/$(OUTPUT): GNUmakefile linker-scripts/$(ARCH).lds $(OBJ) $(RUST_LIB) $(RUST_LIBS)
	mkdir -p "$(dir $@)"
	$(LD) $(LDFLAGS) $(OBJ) $(RUST_LIB) $(RUST_LIBS) -o $@

# Compilation rules for *.c files.
obj-$(ARCH)/%.c.o: %.c GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compilation rules for *.S files.
obj-$(ARCH)/%.S.o: %.S GNUmakefile
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Compile the entire Rust crate into a static library.
obj-$(ARCH)/librustkernel.a: $(RUST_CRATE_ROOT) $(RUST_SOURCES) GNUmakefile
	mkdir -p "$(dir $@)"
	$(RUSTC) \
	    --edition=$(RUST_EDITION) \
	    --target=$(RUST_TARGET) \
	    --crate-type=staticlib \
	    -C panic=abort \
	    -C opt-level=2 \
	    -C debuginfo=2 \
	    -C relocation-model=static \
	    $(RUSTFLAGS) \
	    $(RUST_CRATE_ROOT) \
	    -o $@

ifeq ($(ARCH),x86_64)
# Compilation rules for *.asm (nasm) files.
obj-$(ARCH)/%.asm.o: %.asm GNUmakefile
	mkdir -p "$(dir $@)"
	nasm $(NASMFLAGS) $< -o $@
endif

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf bin-$(ARCH) obj-$(ARCH)

# Remove everything built and generated including downloaded dependencies.
.PHONY: distclean
distclean:
	rm -rf bin-* obj-* .deps-obtained .cache compile_commands.json freestanding-c-hdrs cc-runtime limine-protocol

# Install the final built executable to its final on-root location.
.PHONY: install
install: all
	install -d "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)"
	install -m 644 bin-$(ARCH)/$(OUTPUT) "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)/$(OUTPUT)-$(ARCH)"

# Try to undo whatever the "install" target did.
.PHONY: uninstall
uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)/$(OUTPUT)-$(ARCH)"

