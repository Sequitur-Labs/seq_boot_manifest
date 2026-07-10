
CC      = $(CROSS_COMPILE)gcc
LD      = $(CROSS_COMPILE)ld
AR      = $(CROSS_COMPILE)ar
NM      = $(CROSS_COMPILE)nm
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
READELF = $(CROSS_COMPILE)readelf


MANIFEST_SRC = $(wildcard *.c)
MANIFEST_OBJ = $(patsubst %.c,%.o,$(MANIFEST_SRC))

MANIFEST_INC = $(wildcard *.h)

OBJDIR = obj
OUTDIR = out

library: $(OUTDIR)/libseq_manifest.a $(LIBINCLUDE)


$(LIBINCLUDE): seq_list.h seq_manifest.h $(OUTDIR)
	cp seq_list.h $(LIBINCLUDE)
	cp seq_manifest.h $(LIBINCLUDE)

$(OUTDIR)/libseq_manifest.a: $(MANIFEST_OBJ) $(OUTDIR)
	$(AR) rcs $(OUTDIR)/libseq_manifest.a $(MANIFEST_OBJ)

$(OBJDIR)/%.o : %.c $(MANIFEST_INC)
	$(CC) -std=c99 -g -c $< -o $@

$(MANIFEST_OBJ): $(OBJDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OUTDIR):
	mkdir -p $(OUTDIR)
	mkdir -p $(OUTDIR)/include


clean:
	rm -rf $(OBJDIR)
	rm -rf $(OUTDIR)



