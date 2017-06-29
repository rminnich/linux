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

subdir- := rm

obj-y += init.o
obj-y += rmpiggy.o
obj-$(CONFIG_LINUXBIOS) += smmpiggy.o
obj-$(CONFIG_LINUXBIOS) += i82801ix.o

# since rmpiggy is always built we can make smmpiggy
# depend on it.
$(obj)/smmpiggy.o: $(obj)/rm/smm.bin

$(obj)/rm/smm.bin: FORCE
	$(Q)$(MAKE) $(build)=$(obj)/rm $@

$(obj)/rmpiggy.o: $(obj)/rm/realmode.bin

$(obj)/rm/realmode.bin: FORCE
	$(Q)$(MAKE) $(build)=$(obj)/rm $@


