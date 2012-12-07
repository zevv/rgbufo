
all: rgbufo.sch rgbufo.pcb

.PHONY: rgbufo.sch

rgbufo.sch:
	refdes_renum rgbufo.sch

rgbufo.pcb: rgbufo.sch
	gsch2pcb --use-files --elements-dir footprints rgbufo.sch

clean:
	rm -f *.gbr *.org *.cnc *~ *.bak* *.png *.cmd *.net
