#!/usr/bin/env python3

import time
import contextlib
import subprocess
import dhcpctl


def generate_leases(server, port, subnet, start, end, ping):
    with contextlib.closing(dhcpctl.Connection(server, port)) as dhcp:
        print("IN")

        for i in range(start, end):
            ip, ttl, name = dhcp.get_lease("%s.%d" % (subnet, i))

            if not ttl:
                continue

            status, output = subprocess.getstatusoutput(
                ping % {"ip": ip}
            )

            if not status:
                diff = int(time.time()) - ttl
                hours = int((diff / 60) / 60)
                mins = int((diff / 60) % 60)

                yield "ip=%s, uptime=(%dh, %dm), name=%s" % (
                    ip,
                    hours,
                    mins,
                    name
                )


if __name__ == "__main__":
    server = "192.168.1.2"
    port = 20999
    subnet = "192.168.1"
    start = 50
    end = 200
    ping = "ping -q -c 1 %(ip)s &>/dev/null"

    for lease in generate_leases(server, port, subnet, start, end, ping):
        print(lease)
