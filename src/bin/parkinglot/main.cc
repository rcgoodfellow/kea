// Copyright (C) 2009  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

// $Id$

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <set>
#include <iostream>

#include <dns/buffer.h>
#include <dns/name.h>
#include <dns/rrset.h>
#include <dns/message.h>

#include "common.h"

using std::string;
using std::pair;

using namespace isc::dns;
using isc::dns::Rdata::IN::A;
using isc::dns::Rdata::IN::AAAA;
using isc::dns::Rdata::Generic::NS;

const string PROGRAM = "parkinglot";
const int DNSPORT = 53;

static void
usage() {
        std::cerr << "Usage: parkinglot [-p port]" << std::endl;
        exit(1);
}

typedef pair<string, void*> Record;
typedef std::set<string> ZoneSet;
ZoneSet zones;

static void
serve(string zone) {
    zones.insert(zone);
}

static void
init_db() {
    serve("jinmei.org");
    serve("nuthaven.org");
    serve("isc.org");
    serve("sisotowbell.org");
    serve("flame.org");
}

static int
start_server(int port) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        fatal("failed to open socket");

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    socklen_t sa_len = sizeof(sin);
    struct sockaddr* sa = static_cast<struct sockaddr*>((void*)&sin);

    if (bind(s, sa, sa_len) < 0)
        return (-1);

    return (s);
}

static void
run_server(int s) {
    while (true) {
        Message msg;
        struct sockaddr_storage ss;
        socklen_t sa_len = sizeof(ss);
        struct sockaddr* sa = static_cast<struct sockaddr*>((void*)&ss);
        int cc = msg.getBuffer().recvFrom(s, sa, &sa_len);
        if (cc > 0) {
            try {
                msg.fromWire();
            } catch (...) {
                std::cerr << "parse failed" << std::endl;
                continue;
            }

            std::cout << "received a message:\n" << msg.toText() << std::endl;

            if (msg.getSection(SECTION_QUESTION).size() != 1)
                continue;

            msg.makeResponse();
            msg.setAA(true);

            RRsetPtr query = msg.getSection(SECTION_QUESTION)[0];

            if (zones.find(query->getName().toText(true)) != zones.end()) {
                Rdata::RdataPtr ns1, ns2, ns3;
                ns1 = Rdata::RdataPtr(new NS("ns1.parking.com"));
                ns2 = Rdata::RdataPtr(new NS("ns2.parking.com"));
                ns3 = Rdata::RdataPtr(new NS("ns3.parking.com"));

                msg.setRcode(Message::RCODE_NOERROR);
                RRset* nsset = new RRset(query->getName(), query->getClass(),
                                         RRType::NS, TTL(3600));

                nsset->addRdata(ns1);
                nsset->addRdata(ns2);
                nsset->addRdata(ns3);

                section_t section;
                if (query->getType() == RRType::NS)
                    section = SECTION_ANSWER;
                else
                    section = SECTION_AUTHORITY;
                
                msg.addRRset(section, RRsetPtr(nsset));

                if (query->getType() == RRType::A) {
                    msg.addRR(SECTION_ANSWER,
                             RR(query->getName(), query->getClass(),
                                RRType::A, TTL(3600),
                                Rdata::RdataPtr(new A("127.0.0.1"))));
                } else if (query->getType() == RRType::AAAA) {
                    msg.addRR(SECTION_ANSWER,
                             RR(query->getName(), query->getClass(),
                                RRType::AAAA, TTL(3600),
                                Rdata::RdataPtr(new AAAA("::1"))));
                }
            } else {
                msg.setRcode(Message::RCODE_NXDOMAIN);
                // should add SOA to the authority section, but not implemented.
            }

            msg.toWire();
            std::cout << "sending a response (" <<
                boost::lexical_cast<string>(msg.getBuffer().getSize())
                      << " bytes):\n" << msg.toText() << std::endl;
            msg.getBuffer().sendTo(s, *sa, sa_len);
        }
    }
}

int
main(int argc, char* argv[])
{
    Message msg;
    int ch;
    int port = DNSPORT;
    bool err = false;

    while ((ch = getopt(argc, argv, "p:")) != -1) {
        switch (ch) {
        case 'p':
            port = atoi(optarg);
            break;
        case '?':
        default:
            err = true;
        }
    }

    if (err || (argc - optind) > 0)
        usage();

    // initialize DNS database
    init_db();

    // start the server
    int s = start_server(port);

    // main server loop
    run_server(s);

    return (0);
}
