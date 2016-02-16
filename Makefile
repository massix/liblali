SUBDIRS = flate json11 web

.PHONY: clean flate/libflate.so json11/libjson11.so web/liblali.so

all: flate/libflate.so json11/libjson11.so web/liblali.so

flate/libflate.so:
	$(MAKE) -C flate all

json11/libjson11.so:
	$(MAKE) -C json11 all

web/liblali.so:
	$(MAKE) -C web all

clean:
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done
