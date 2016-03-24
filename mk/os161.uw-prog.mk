# Tools to generate files useful for debugging.

# UW - TBB June 28, 2013
#      Added this ldscript to get data segment separated from the text segment
#      the way it was before we upgraded to the new version of the toolchain.
#      It moves the data segment to 0x10000000 and a few other minor things.
#      If it causes problems we should be able to just comment out this line.
UWLDSCRIPT = -T $(TOP)/mk/uw-prog-ldscript

EXTRAS = $(MYBUILDDIR)/$(PROG).readelf $(MYBUILDDIR)/$(PROG).nm \
         $(MYBUILDDIR)/$(PROG).objdump $(MYBUILDDIR)/$(PROG).asm

extras: $(EXTRAS)

echo:
	echo $(MYBUILDDIR)

$(MYBUILDDIR)/$(PROG).readelf:	$(PROG).c $(MYBUILDDIR)/$(PROG)
	cs350-readelf -a $(MYBUILDDIR)/$(PROG) > $(MYBUILDDIR)/$(PROG).readelf

$(MYBUILDDIR)/$(PROG).objdump:	$(PROG).c $(MYBUILDDIR)/$(PROG)
	cs350-objdump -s $(MYBUILDDIR)/$(PROG) > $(MYBUILDDIR)/$(PROG).objdump

$(MYBUILDDIR)/$(PROG).asm:	$(PROG).c $(MYBUILDDIR)/$(PROG)
	cs350-objdump -d $(MYBUILDDIR)/$(PROG) > $(MYBUILDDIR)/$(PROG).asm

$(MYBUILDDIR)/$(PROG).nm:	$(PROG).c $(MYBUILDDIR)/$(PROG)
	cs350-nm -n $(MYBUILDDIR)/$(PROG) > $(MYBUILDDIR)/$(PROG).nm

clean:
	-/bin/rm $(EXTRAS)	

.PHONY: clean echo
