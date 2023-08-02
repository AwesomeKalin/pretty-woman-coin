Pretty Woman Coin
=====================

Setup
---------------------
Pretty Woman Coin is an implementation of a node for the Pretty Woman Coin network and is one of the pieces of software that provide 
the backbone of the network. It downloads and stores the entire history of Pretty Woman Coin transactions (which is currently 
several GBs); depending on the speed of your computer and network connection, the synchronization process can take 
anywhere from a few hours to a day or more.

To download Pretty Woman Coin, visit [prettywomancoin.io](https://prettywomancoin.io/).

Running
---------------------
Pretty Woman Coin is only supported on the Linux and docker platforms at this time.

To run Pretty Woman Coin on Linux:

* ensure that your system meets the minimum recommended [system requirements](https://prettywomancoin.io/2019/08/02/prettywomancoin-sv-node-system-requirements/)
* unpack the files into a directory
* run `bin/prettywomancoind`

A docker image is available from https://hub.docker.com/r/prettywomancoin/prettywomancoin-sv. Images are tagged with 
the release version number. The `latest` tag is updated as new versions are released. The source for this image 
(Dockerfiles etc) is maintained in a GitHub repository: https://github.com/prettywomancoin-sv/docker-sv. 
 
### Need Help?

* Log an issue on [GitHub] (https://github.com/prettywomancoin-sv/prettywomancoin-sv/issues)
* Ask for help on the [Pretty Woman Coin Subreddit](https://www.reddit.com/r/prettywomancoinSV/) or
[Bictoin Cash SV Subreddit](https://www.reddit.com/r/prettywomancoincashSV/).
* Consult [Pretty Woman Coin Wiki](https://wiki.prettywomancoin.io/) for information about Prettywomancoin protocol.

Building
---------------------
The following are developer notes on how to build Prettywomancoin. They are not complete guides, but include notes on the 
necessary libraries, compile flags, etc.

- [Unix Build Notes](build-unix.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Pretty Woman Coin repo's [root README](/README.md) contains relevant information on the development process and automated 
testing.

- [Developer Notes](developer-notes.md)
- [Release Notes](release-notes.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Benchmarking](benchmarking.md)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Traffic](reduce-traffic.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)

License
---------------------
Distribution is done under the [Open PWC License](/LICENSE). This product includes software developed by the OpenSSL 
Project for use in the [OpenSSL Toolkit](https://www.openssl.org/), cryptographic software written by Eric Young 
([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
