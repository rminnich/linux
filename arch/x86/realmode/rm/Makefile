#
# arch/x86/realmode/Makefile
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
#
KASAN_SANITIZE			:= n
OBJECT_FILES_NON_STANDARD	:= y

# Prevents link failures: __sanitizer_cov_trace_pc() is not linked in.
KCOV_INSTRUMENT		:= n

always := realmode.bin realmode.relocs

wakeup-objs	:= wakeup_asm.o wakemain.o video-mode.o
wakeup-objs	+= copy.o bioscall.o regs.o
# The link order of the video-*.o modules can matter.  In particular,
# video-vga.o *must* be listed first, followed by video-vesa.o.
# Hardware-specific drivers should follow in the order they should be
# probed, and video-bios.o should typically be last.
wakeup-objs	+= video-vga.o
wakeup-objs	+= video-vesa.o
wakeup-objs	+= video-bios.o

realmode-y			+= header.o
realmode-y			+= trampoline_$(BITS).o
realmode-y			+= stack.o
realmode-y			+= reboot.o
smm-$(CONFIG_LINUXBIOS)	+= smmrelocate.o
realmode-$(CONFIG_LINUXBIOS)	+= smmhandler.o
realmode-$(CONFIG_ACPI_SLEEP)	+= $(wakeup-objs)

targets	+= $(realmode-y) $(smm-y)

REALMODE_OBJS = $(addprefix $(obj)/,$(realmode-y))
SMM_OBJS = $(addprefix $(obj)/,$(smm-y))

sed-pasyms := -n -r -e 's/^([0-9a-fA-F]+) [ABCDGRSTVW] (.+)$$/pa_\2 = \2;/p'

quiet_cmd_pasyms = PASYMS  $@
      cmd_pasyms = $(NM) $(filter-out FORCE,$^) | \
		   sed $(sed-pasyms) | sort | uniq > $@

targets += pasyms.h
$(obj)/pasyms.h: $(REALMODE_OBJS) FORCE
	$(call if_changed,pasyms)

targets += realmode.lds

$(obj)/realmode.lds: $(obj)/pasyms.h

LDFLAGS_realmode.elf := --emit-relocs -T
CPPFLAGS_realmode.lds += -P -C -I$(objtree)/$(obj)

targets += realmode.elf
$(obj)/realmode.elf: $(obj)/realmode.lds $(REALMODE_OBJS) FORCE
	$(call if_changed,ld)

OBJCOPYFLAGS_realmode.bin := -O binary

targets += realmode.bin
$(obj)/realmode.bin: $(obj)/realmode.elf $(obj)/realmode.relocs FORCE
	$(call if_changed,objcopy)

quiet_cmd_relocs = RELOCS  $@
      cmd_relocs = arch/x86/tools/relocs --realmode $< > $@

targets += realmode.relocs
$(obj)/realmode.relocs: $(obj)/realmode.elf FORCE
	$(call if_changed,relocs)

# --- smm ---
sed-smmpasyms := -n -r -e 's/^([0-9a-fA-F]+) [ABCDGRSTVW] (.+)$$/pa_\2 = \2;/p'

quiet_cmd_smmpasyms = SMMPASYMS  $@
      cmd_smmpasyms = $(NM) $(filter-out FORCE,$^) | \
		   sed $(sed-smmpasyms) | sort | uniq > $@

targets += smm.lds

targets += smmpasyms.h
$(obj)/smmpasyms.h: $(REALMODE_OBJS) FORCE
	$(call if_changed,smmpasyms)

targets += realmode.lds smm.lds
$(obj)/smm.lds: $(obj)/smmpasyms.h

LDFLAGS_smm.elf := --emit-relocs -T
CPPFLAGS_smm.lds += -P -C -I$(objtree)/$(obj)

targets += smm.elf
$(obj)/smm.elf: $(obj)/smm.lds $(SMM_OBJS) FORCE
	$(call if_changed,ld)

OBJCOPYFLAGS_smm.bin := -O binary

targets += smm.bin
$(obj)/smm.bin: $(obj)/smm.elf $(obj)/smm.relocs FORCE
	$(call if_changed,objcopy)

quiet_cmd_relocs = RELOCS  $@
      cmd_relocs = arch/x86/tools/relocs --realmode $< > $@

targets += smm.relocs
$(obj)/smm.relocs: $(obj)/smm.elf FORCE
	$(call if_changed,relocs)

# ---------------------------------------------------------------------------

KBUILD_CFLAGS	:= $(REALMODE_CFLAGS) -D_SETUP -D_WAKEUP \
		   -I$(srctree)/arch/x86/boot
KBUILD_AFLAGS	:= $(KBUILD_CFLAGS) -D__ASSEMBLY__
GCOV_PROFILE := n
UBSAN_SANITIZE := n
