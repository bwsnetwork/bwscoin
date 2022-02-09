Proposed PoUW Consensus Mechanism
===========================================

The contributing developers of BWS Coin have released a white paper proposing a Proof of Useful Work (PoUW) consensus mechanism. Read the white paper [here](https://valdi.ai/whitepaper).

BWS Coin Core integration/staging tree
=====================================

https://valdi.ai

What is Valdi Labs and BWS Coin?
----------------

The BWS Blockchain Protocol (BWS blockchain) enables a decentralized AI economy
where application developers can create products and services that will be beneficial
to the BWS ecosystem and users can contribute their BWS data to improve and enhance the
platform's AI algorithms. In addition, companies and developers can easily create
their own token on top of the BWS blockchain to facilitate interaction and transaction
in their own unique experiences.

BWS Coin is a digital currency that enables instant payments to anyone, anywhere in the world.
BWS Coin uses peer-to-peer technology to operate with no central authority: managing
transactions and issuing money are carried out collectively by the network.
BWS Coin Core is the name of the open-source software which enables the use of this currency.

Read the whitepapers [here](https://valdi.ai/whitepaper).

License
-------

BWS Coin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/valdi-labs/bwscoin/tags) are created
regularly to indicate new official, stable release versions of BWS Coin Core.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written
in Python, that are run automatically on the build server.
These tests can be run (if the [test dependencies](/test) are installed) with: `test/functional/test_runner.py`

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Community Resources
-------------------

### English

- [Twitter](https://twitter.com/valdilabs)
- [Medium](https://valdi.medium.com/)
- [YouTube](https://www.youtube.com/channel/UCND0qYcW98slzhYxCPK54WA)
- [Discord](https://discord.gg/Gb62FKnd9A)
