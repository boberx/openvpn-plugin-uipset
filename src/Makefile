CC = gcc
BUILDDIR = build

PLGNSRC = openvpn-plugin-uipset.c common.c avpfile.c
PLGNOBJ = $(patsubst %.c,$(BUILDDIR)/%.o,$(PLGNSRC))
PLGNSNM = openvpn-plugin-uipset
PLGNCFL = -O2 -fPIC -Wall -Wextra $(CFLAGS) -I/usr/include/uipset -I/usr/include/nanopb

all: dir $(BUILDDIR)/$(PLGNSNM).so

$(BUILDDIR)/$(PLGNSNM).so: $(PLGNOBJ)
	$(CC) $(PLGNCFL) -shared -Wl,-soname,$(LHTTPDSNM).so $^ -o $@ -luipset

$(PLGNOBJ): $(BUILDDIR)/%.o : %.c dir
	$(CC) $(PLGNCFL) -c $< -o $@

dir:
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)
