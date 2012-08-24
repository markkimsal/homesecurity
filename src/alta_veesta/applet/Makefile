compile: do_compile
-e do_compile: do_build show_size
do_build: applet/$(TARGET).hex
applet/$(TARGET).hex: applet/$(TARGET).elf

ARDMAKE_BOARD=applet/board

applet/$(TARGET).elf: applet/$(TARGET).cpp applet/core.a
	$(CXX) $(ALL_CXXFLAGS) -Wl,--gc-sections $(LDFLAGS) -L. -Lapplet/ -o $@ $< applet/core.a
	@chmod a-x $@ >/dev/null 2>&1 || true

applet/$(TARGET).cpp: $(TARGET).$(EXT) $(ARDUINO)/main.cxx $(ARDUINO)/WProgram.h $(ARDMAKE_BOARD)
	echo '#include "WProgram.h"' >$@
	@echo '#line 1 "$<"' >>$@
	cat $(TARGET).$(EXT) >>$@
	@echo '#line 1 "$(ARDUINO)/main.cxx"' >>$@
	cat $(ARDUINO)/main.cxx >>$@

show_size:
	@echo
	@echo Program size:
	@$(HEXSIZE) | awk -v m="$(MAX_SIZE)" '{print;if(NR^1){s=$$4}} \
    END {printf("\n%d/%d bytes (%.1f%% of capacity, %d bytes left)\n\n",\
    s,m,s*100.0/m,m-s);}'

upload_autoreset: do_autoreset upload unreset

do_autoreset:
	@echo Sending reset to prepare for upload...
	( stty hupcl; sleep 0.1; true ) <$(PORT) 2>/dev/null
	@echo

unreset:
	@stty -hupcl <$(PORT) 2>/dev/null || true

$(OBJ): $(ARDMAKE_BOARD)
$(DEPS): $(ARDMAKE_BOARD)

$(APPC): applet/%.o: %.c
	$(CC) -c $(ALL_CFLAGS) -o $@ $<

$(APPCXX): applet/%.o: %.cpp
	$(CXX) -c $(ALL_CXXFLAGS) -o $@ $<

$(APPA): applet/%.o: %.S
	$(CC) -c $(ALL_ASFLAGS) -o $@ $<

$(APPC:.o=.d): applet/%.d: %.c
	$(CC) -M $(ALL_CFLAGS) $< | sed 's;^[^:]*:;applet/$*.o applet/$*.d:;' >$@

$(APPCXX:.o=.d): applet/%.d: %.cpp
	$(CXX) -M $(ALL_CXXFLAGS) $< | sed 's;^[^:]*:;applet/$*.o applet/$*.d:;' >$@

$(APPA:.o=.d): applet/%.d: %.S
	$(CC) -M $(ALL_ASFLAGS) $< | sed 's;^[^:]*:;applet/$*.o applet/$*.d:;' >$@

applet/$(TARGET).d: applet/$(TARGET).cpp

vpath %.c applet/ $(sort $(dir $(OBJC)))
vpath %.cpp applet/ $(sort $(dir $(OBJCXX)))
vpath %.S applet/ $(sort $(dir $(OBJA)))

include $(DEPS)
