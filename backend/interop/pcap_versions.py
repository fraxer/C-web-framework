#!/usr/bin/env python3
"""Which QUIC versions appear in which long-header packets, straight from a pcap.

Written for one job: answering the quic-interop-runner's `v2` case
(docs/http3/08 §17f) on a machine where its own check cannot run. That check
needs pyshark, pyshark needs tshark, and tshark needs root to install --
so the runner executes the exchange perfectly and then dies at the verdict.

The four questions the case asks are the version of the client's Initial
packets, of the server's Initial packets, and of each side's Handshake packets.
Every one is answerable from the long header without decrypting anything:
RFC 8999 fixes the first byte, the version and the two connection ids for every
QUIC version there will ever be. That is the whole reason this fits in a
hundred lines while the runner needs a protocol dissector.

Usage, after a run with the simulator's /logs mounted out:

    python3 interop/pcap_versions.py trace_node_left.pcap 193.167.0.100

`v2` passes when the client's Initial carries 00000001, the server's carries
6b3343cf, and each side's Handshake carries 6b3343cf and nothing else.
"""
import struct
import sys

V1 = 0x00000001
V2 = 0x6B3343CF

# Long-header type codes differ per version (RFC 9369 §3.2), so the name of a
# packet is a function of both the type bits and the version.
TYPES = {
    V1: {0: "initial", 1: "0rtt", 2: "handshake", 3: "retry"},
    V2: {1: "initial", 2: "0rtt", 3: "handshake", 0: "retry"},
}


def _varint(buf, p):
    if p >= len(buf):
        return None, p
    prefix = buf[p] >> 6
    n = 1 << prefix
    if p + n > len(buf):
        return None, p
    value = buf[p] & 0x3F
    for k in range(1, n):
        value = (value << 8) | buf[p + k]
    return value, p + n


def packets(payload):
    """Every long-header QUIC packet coalesced in one UDP datagram.

    Walking all of them rather than stopping at the first is the difference
    between answering "which versions are in this trace" and answering it
    correctly: the check that matters most -- that a side used exactly one
    version for its Handshake packets -- is precisely a question about a stray
    packet coalesced behind a well-formed one.
    """
    off = 0
    while off + 7 <= len(payload):
        first = payload[off]
        if not first & 0x80:
            return                      # short header: no version, nothing to say
        version = struct.unpack(">I", payload[off + 1 : off + 5])[0]
        p = off + 5
        for _ in range(2):              # DCID then SCID, each length-prefixed
            if p >= len(payload):
                return
            p += 1 + payload[p]
        name = TYPES.get(version, {}).get((first & 0x30) >> 4)
        yield version, name

        # Past this packet to the next one. A Retry has neither Length nor
        # packet number and ends the datagram; anything else carries a Length
        # that covers the packet number and the protected payload.
        if name == "retry" or name is None:
            return
        if name == "initial":
            token_len, p = _varint(payload, p)
            if token_len is None:
                return
            p += token_len
        length, p = _varint(payload, p)
        if length is None:
            return
        off = p + length


def udp_payloads(path):
    """(src_ip, dst_ip, payload) for every UDP datagram in a pcap or pcapng."""
    data = open(path, "rb").read()
    magic = data[:4]
    if magic in (b"\xd4\xc3\xb2\xa1", b"\xa1\xb2\xc3\xd4"):
        yield from _pcap(data, magic == b"\xd4\xc3\xb2\xa1")
    elif magic == b"\x0a\x0d\x0d\x0a":
        yield from _pcapng(data)
    else:
        raise SystemExit("не pcap и не pcapng: %s" % path)


def _frame(buf):
    if len(buf) < 34 or buf[12:14] != b"\x08\x00":
        return None                     # not IPv4 over Ethernet
    ihl = (buf[14] & 0x0F) * 4
    if buf[14 + 9] != 17:               # not UDP
        return None
    src = ".".join(str(b) for b in buf[26:30])
    dst = ".".join(str(b) for b in buf[30:34])
    udp = 14 + ihl
    return src, dst, buf[udp + 8 :]


def _pcap(data, little):
    end = "<" if little else ">"
    off = 24
    while off + 16 <= len(data):
        _, _, caplen, _ = struct.unpack(end + "IIII", data[off : off + 16])
        buf = data[off + 16 : off + 16 + caplen]
        off += 16 + caplen
        got = _frame(buf)
        if got:
            yield got


def _pcapng(data):
    off = 0
    while off + 12 <= len(data):
        btype, blen = struct.unpack("<II", data[off : off + 8])
        if blen < 12 or off + blen > len(data):
            return
        if btype == 6:                  # Enhanced Packet Block
            caplen = struct.unpack("<I", data[off + 20 : off + 24])[0]
            got = _frame(data[off + 28 : off + 28 + caplen])
            if got:
                yield got
        off += blen


def main():
    if len(sys.argv) < 3:
        raise SystemExit("usage: pcapver.py FILE.pcap CLIENT_IP")
    path, client_ip = sys.argv[1], sys.argv[2]
    seen = {}
    for src, dst, payload in udp_payloads(path):
        who = "client" if src == client_ip else "server"
        for version, name in packets(payload):
            if name:
                seen.setdefault((who, name), set()).add(version)
    for key in sorted(seen):
        print("%-6s %-9s -> %s" % (key[0], key[1],
                                   ", ".join("%08x" % v for v in sorted(seen[key]))))


if __name__ == "__main__":
    main()
