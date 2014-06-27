PYTHON_CFLAGS := $(shell pkg-config --cflags python3)
PYTHON_LIBS   := $(shell pkg-config --libs python3)
DHCPCTL_LIBS  := -ldhcpctl -lomapi -lisc-export -ldns-export # -ldst

dhcpctl.so: dhcpctl.c
	@gcc -fPIC -o $(@) -shared $(<) -W -Wall -Wno-unused-parameter \
	$(PYTHON_CFLAGS) $(PYTHON_LIBS) $(DHCPCTL_LIBS)

clean:
	@rm -f dhcpctl.so

