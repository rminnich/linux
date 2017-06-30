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
# obj-$(CONFIG_LINUXBIOS) += smmpiggy.o
obj-$(CONFIG_LINUXBIOS) += i82801ix.o

$(obj)/smmpiggy.o: $(obj)/rm/smm.bin
	echo "make smmpiggy?"

$(obj)/rm/smm.bin: FORCE
	echo "create smm.bin @ is $@"
	echo $(MAKE) $(build)=$(obj)/rm $@
	$(MAKE) $(build)=$(obj)/rm $@
	echo "createed smm.bin"

$(obj)/rmpiggy.o: $(obj)/rm/realmode.bin

$(obj)/rm/realmode.bin: FORCE
	$(Q)$(MAKE) $(build)=$(obj)/rm $@


