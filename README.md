# Reliable Multicast File Transfer Client (RMFTC)

### Application-Level Reliability over UDP Multicast (C \| GTK3 \| Pthreads)

![C](https://img.shields.io/badge/C-ANSI%20C-blue)
![GTK3](https://img.shields.io/badge/GUI-GTK3-green)
![UDP](https://img.shields.io/badge/Protocol-UDP%20Multicast-orange)
![Threads](https://img.shields.io/badge/Concurrency-Pthreads-red)
![Build](https://img.shields.io/badge/Build-Makefile-lightgrey)

------------------------------------------------------------------------

## Overview

RMFTC implements an **application-level reliability protocol over UDP
multicast**.

The system reconstructs complete files transmitted through an unreliable
transport channel by detecting packet loss and tracking received blocks
using a bitmask-based mechanism.

Instead of relying on TCP, this project demonstrates how reliability,
ordering, and completion detection can be implemented manually on top of
UDP while preserving multicast scalability.

Key technical areas demonstrated:

-   Low-level socket programming in C
-   UDP multicast networking
-   Custom reliability mechanisms
-   POSIX multithreading (pthreads)
-   File reconstruction from fragmented data
-   GTK3 graphical interface integration

------------------------------------------------------------------------

## Why Multicast Instead of TCP?

TCP cannot efficiently distribute the same file to multiple receivers
simultaneously, as each connection requires an independent stream and
retransmissions.

UDP multicast allows a sender to transmit data **once**, while multiple
receivers subscribe to the stream.

The challenge is reliability.

This project addresses that problem by implementing reliability
mechanisms at the application layer, enabling scalable one-to-many file
distribution.

Potential real-world applications:

-   Firmware distribution to embedded systems
-   IPTV and media streaming
-   Software deployment in controlled enterprise networks
-   Industrial network update systems

------------------------------------------------------------------------

## Architecture

    .
    ├── main.c                # Program entry point
    ├── sock.c / sock.h       # Multicast socket management
    ├── receiver_th.c         # Packet reception thread
    ├── bitmask.c             # Packet tracking and loss detection
    ├── file.c                # File reconstruction logic
    ├── callbacks.c           # GTK signal handlers
    ├── gui_g3.c              # GUI implementation
    ├── fmulticast_client.glade
    ├── Makefile

### Core Modules

**Socket Layer** - Creates UDP socket - Joins multicast group - Receives
datagrams

**Receiver Thread** - Continuously listens for packets - Processes
incoming fragments - Updates reception state

**Reliability Layer (Bitmask-Based Tracking)** - Tracks received packet
blocks - Detects missing fragments - Determines transfer completion

**File Assembly** - Writes received data to correct offsets - Produces
final reconstructed file

**Graphical Interface (GTK3)** - Built using Glade - Allows
configuration and monitoring - Displays transfer progress

------------------------------------------------------------------------

## Reliability Strategy

Because UDP multicast provides no delivery guarantees, RMFTC implements:

1.  Block identification per packet
2.  Bitmask tracking of received blocks
3.  Missing fragment detection
4.  Completion validation before file finalization

This preserves multicast efficiency while ensuring file integrity.

------------------------------------------------------------------------

## Concurrency Model

-   Main thread: GTK event loop
-   Receiver thread: network packet processing

This ensures the graphical interface remains responsive during file
transfers.

------------------------------------------------------------------------

## Build Instructions

### Requirements

-   GCC
-   GTK3 development libraries
-   Make

Install on Ubuntu/Debian:

``` bash
sudo apt install build-essential libgtk-3-dev
```

### Compile

``` bash
make
```

### Run

``` bash
./fmulticast_client
```

------------------------------------------------------------------------

## Networking Requirements

-   Multicast-enabled network
-   UDP traffic allowed by firewall
-   Sender transmitting to the same multicast group and port

------------------------------------------------------------------------

## Educational Scope

Relevant academic domains:

-   Computer Networks
-   Distributed Systems
-   Operating Systems

Core concepts covered:

-   Multicast communication
-   Reliability over unreliable transport
-   Thread synchronization
-   Fragmented file reconstruction

------------------------------------------------------------------------

## Future Improvements

-   NACK-based retransmission mechanism
-   Sender-side implementation
-   Transfer performance metrics
-   Multiple simultaneous transfers
-   Congestion control mechanisms

------------------------------------------------------------------------

## 👤 Author

Diogo Neto Filipe\
MSc Student in Electrical and Computer Engineering --- NOVA FCT\
Interests: Computer Networks, Distributed Systems, Cybersecurity

Supervised by\
Prof. Luís Bernardo --- NOVA FCT

------------------------------------------------------------------------

## License

MIT License
