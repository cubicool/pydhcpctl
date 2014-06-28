dhcpctl.so: dhcpctl.c
	@python3 setup.py build

clean:
	@rm -f dhcpctl.cpython*
	@rm -rf build
