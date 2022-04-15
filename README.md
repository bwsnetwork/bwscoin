Blockchain Web Services (BWS)
=============================

![bws-logo-with-text-dark-background-banner](https://user-images.githubusercontent.com/16344190/153721488-78357864-b26d-480b-8993-9b0d39c2b5d2.png)

### This codebase is under development. There is currently no stable release.

The BWS Blockchain code has been preemptively open sourced for the sake of transparency as contributors progress toward their goals. Comments and questions are welcome via GitHub Issues. You may submit pull requests, but their review and potential incorporation will be delayed until contribution guidelines are made available (see [below](#contributing)).

What is the BWS Blockchain?
---------------------------

The BWS Blockchain is a PoW-based blockchain powered by Nakamoto-style mining but where nonces are verifiably dervied from custom computational tasks submitted by peers. Consensus is achieved using a novel Proof of Useful Work (PoUW) protocol. Supported computational tasks are embedded at the protocol level and are largely derived from the TensorFlow API.

How does BWS work?
------------------

Machine Learning powers the BWS Blockchain. BWS Miners generate blocks for the blockchain as a side effect of working on user tasks. Customers choose from a variety of ML job templates (e.g., DNN, GAN) on which to base their tasks. Each block mined is cryptographically linked to a piece of useful work. Mining and task processing are linked by design, so the blockchain is supported with minimal additional energy expenditure. With the network driven by voluntary public participation, BWS can offer more compute at lower cost compared to centralized cloud computing providers.

## BWS Network Roles

There are six primary roles that make up the BWS Blockchain ecosystem. None of the roles is mutually exclusive; in fact, a single party could adopt one, several, or even all of the roles simultaneously.

- **Clients** are BWS users who pay, in BWS Coin, to submit tasks to the BWS Blockchain for computation. Clients can submit tasks in one of two ways: through delegation via the BWS Portal, or by running their own full node client.
- **Miners** actively monitor for new task submissions from clients. When a new task is requested, miners work through the requisite computations collaboratively, sharing their intermediate results with one another along the way. On BWS, computational tasks are typically structured to be run over thousands of iterations. Upon the completion of an iteration, each miner has the option of generating a block candidate for acceptance to the BWS Blockchain. If a miner's block candidate is accepted, they are rewarded with newly minted BWS Coins. Note that whether a miner's block candidate is accepted or not has no bearing on their ability to earn BWS Coin from task and transaction fees.
- **Supervisors** act as referees, overseeing computational tasks that are being actively worked on by groups of miners. These supervisors record all communications between miners and work to protect against any malicious behaviors.
- **Evaluators** are responsible for assessing the final results of computational tasks, relaying optimal results to clients, and paying all participating nodes.
- **Verifiers** ensure that candidate blocks are valid prior to being added to the blockchain. In order to validate a candidate block, verifiers must repeat the single iteration of the task that produced it. This is to protect against malicious miners who attempt to produce blocks unrelated to clients' requested computational tasks.
- **Peers** keep a local copy of the blockchain and relay transactions; peers do not actively participate in the creation of new blocks. Robustness and efficiency of the blockchain is increased with more peers.

Community Resources
-------------------

### English

- [Discord](https://discord.gg/Gb62FKnd9A)
- [Twitter](https://twitter.com/bws_network)
- [Medium](https://bwsnetwork.medium.com/)
- [YouTube](https://www.youtube.com/channel/UCND0qYcW98slzhYxCPK54WA)

Contributing
------------

Contribution Guidelines and a Contributor Code of Conduct will be available soon. Please be advised that no PRs will be reviewed or approved until these additional documents are released.

License
-------

BWS Blockchain is released under the terms of the MIT license. See [COPYING](COPYING) for more information or see https://opensource.org/licenses/MIT.
