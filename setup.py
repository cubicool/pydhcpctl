from distutils.core import setup, Extension

mod = Extension(
    "dhcpctl",
    define_macros=[
        ("MAJOR_VERSION", "0"),
        ("MINOR_VERSION", "1")
    ],
    sources=["dhcpctl.c"]
)

setup(
    name="pydhcpctl",
    version="0.1",
    description="ISC DHCPD OMAPI support",
    author="Jeremy Moles",
    author_email="cubicool@gmail.com",
    url="http://github.com/cubicool/pydhcpctl",
    long_description="""
        Wraps libdhcpctl for interfacing with the ISC DHCP server.
    """,
    ext_modules=[mod]
)
