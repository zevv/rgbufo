
SUBDIRS = avr linux android

all: $(SUBDIRS)

.PHONY: $(SUBDIRS)

$(SUBDIRS):
	make -C $@

clean:
	for dir in $(SUBDIRS); do make -C $$dir $@; done
