dist_noinst_DATA=index.html

if USE_GROFF

TARGET=copyman
SUFFIXES = .html .1

copyman:
	cp ../../man/*1 .
	$(MAKE) dar.html dar_slave.html dar_xform.html dar_manager.html dar_cp.html dar_split.html
	touch copyman

.1.html:
	sed -e 's%\-%\\-%g' < ./$< | groff -man -Thtml | sed -e 's%<table%<table border=\"0\"%g' -e 's/&minus;/-/g' > $@

all-local: $(TARGET)

clean-local:
	rm -f $(TARGET) *.1 copyman dar.html dar_slave.html dar_xform.html dar_manager.html dar_cp.html dar_split.html

endif
