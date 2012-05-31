dist_noinst_DATA=index.html

if USE_GROFF

TARGET=dar.html dar_slave.html dar_xform.html dar_manager.html dar_cp.html

%.1:
	cp ../../man/$@ .

%.html: %.1
	sed -e 's%\-%\\-%g' < ./$< | groff -man -Thtml | sed -e 's%<table%<table border=\"0\"%g' -e 's/&minus;/-/g' > $@

all-local: $(TARGET)

clean-local:
	rm -f $(TARGET) *.1

endif
