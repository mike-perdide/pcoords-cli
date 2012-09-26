POD_FILES = $(wildcard *.pod)
MAN_FILES = $(POD_FILES:%.pod=%.1)

all: $(MAN_FILES)

%.1: $(POD_FILES)
	pod2man $^ > $@
	sed -i 's/User Contributed Perl Documentation/Picviz Documentation/' $@

clean:
	rm -f $(MAN_FILES)

