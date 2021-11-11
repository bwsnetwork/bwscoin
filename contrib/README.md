Repository Tools
---------------------

### [Data Sharing](/contrib/data-share) ###

Protocol definition of, and tools for using, the Data Storage Layer of the BWS Blockchain, which allows for trustless data sharing across P2P networks via specialized `OP_RETURN` transactions.

### [Linearize](/contrib/linearize) ###
Construct a linear, no-fork, best version of the blockchain.

### [Qos](/contrib/qos) ###

A Linux bash script that will set up traffic control (tc) to limit the outgoing bandwidth for connections to the BWS Coin network. This means one can have an always-on bwscoind instance running, and another local bwscoind/bwscoin-qt instance which connects to this node and receives blocks from it.

### [Seeds](/contrib/seeds) ###
Utility to generate the pnSeed[] array that is compiled into the client.

### [Key Uncompress](/contrib/key-uncompress) ###
Utilities to convert (uncompress) compressed public and private keys to their corresponding uncompressed forms.
Credit and thanks to:

    https://bitcointalk.org/index.php?topic=644919.msg7205689#msg7205689
    
    https://www.reddit.com/r/Bitcoin/comments/7fptly/how_to_convert_private_key_wif_compressed_start/

Build Tools and Keys
---------------------

### [Debian](/contrib/debian) ###
Contains files used to package bwscoind/bwscoin-qt
for Debian-based Linux systems. If you compile bwscoind/bwscoin-qt yourself, there are some useful files here.

### [MacDeploy](/contrib/macdeploy) ###
Scripts and notes for Mac builds. 
