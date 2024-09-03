# BitTorrent Protocol Simulation using MPI
## Project Overview

The goal of this project is to simulate the BitTorrent peer-to-peer (P2P) file-sharing protocol using MPI (Message Passing Interface). The BitTorrent protocol allows users to distribute and share electronic files over the Internet in a decentralized manner.
## BitTorrent Protocol

BitTorrent is a communication protocol for P2P file sharing that enables users to distribute data and electronic files over the Internet without relying on a centralized server. Instead of downloading a file from a single source server, BitTorrent allows users to join a swarm of nodes, uploading and downloading pieces of a file simultaneously from one another.
### How BitTorrent Works

  - File Segmentation: The file to be shared is divided into multiple segments. As each node receives a new segment, it becomes a source for that segment for other clients, reducing the load on the original source.

  - Decentralized Distribution: With BitTorrent, the file distribution load is shared among those who want the file. The source only needs to send a single copy of the file, but eventually, it can be distributed to an unlimited number of clients.

  - Segment Integrity: Each segment is protected by a cryptographic hash, ensuring any modification to the segment is detected, thus preventing accidental or malicious changes.

  - Non-Sequential Downloads: Segments are downloaded in a non-sequential order and rearranged in the correct order by the BitTorrent client. This allows downloads to be paused and resumed without losing previously downloaded data.

### Roles in BitTorrent

A client in the BitTorrent network can take on one of three roles:

  - Seed: Holds the entire file and only uploads segments to other interested clients.
  - Peer: Holds some segments of a file and both uploads and downloads segments to and from other clients.
  - Leecher: Holds none or only a few segments of a file and only downloads segments from other clients without uploading them.

A client can have different roles for different files simultaneously; for example, it can be a seed for one file and a peer for another.
## BitTorrent Tracker

A BitTorrent tracker is a special server that helps coordinate the communication between clients in the BitTorrent protocol. It keeps track of which clients hold which segments of a file, aiding in the efficient transmission and reassembly of the file. While the initial download relies on the tracker, peer-to-peer communication can continue without it once the download has started.
## Implementation Details
### MPI Simulation

This project simulates the BitTorrent protocol using MPI to handle the communication between different nodes (clients). Each node can take on the roles of seed, peer, or leecher depending on the file it is interacting with. The simulation involves managing the distribution of file segments, coordinating downloads and uploads, and ensuring data integrity through hash verification.

### Running the Project

After compiling the project, you will get an executable named tema3. To run the program, use the following command:

```bash
$ mpirun -np <N> ./tema3
```

Where <N> is the number of MPI tasks that will be launched. The value of <N> must always be greater than or equal to 3.

  - The task with rank 0 will act as the tracker.
  - other tasks will take on the roles of clients.
