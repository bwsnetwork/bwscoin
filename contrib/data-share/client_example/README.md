# Python module for BWS Coin OP_RETURN transactions

Simple Python scripts and API for using OP_RETURN in BWS Coin transactions.

Copyright (c) 2021 Valdi - https://valdi.ai<br>
Copyright (c) ObEN, Inc. - https://oben.me/ <br>
Copyright (c) Coin Sciences Ltd - http://coinsecrets.org/

MIT License


## REQUIREMENTS

* Python 2.5 or later (including Python 3)
* BWS Coin Core 0.9 or later
* requests (2.18.4)

## INSTALLATION
```bash
git clone git@github.com:valdi-labs/bwscoin-txn.git
cd bwscointxn
pip install -r requirements.txt
pip install .
```

## BEFORE YOU START

If you just installed BWS Coin Core, wait for it to download and verify old blocks.<br>
If using as a module, add `from bwscointxn import Transaction` in your Python script file.


## SEND BWS Coin TRANSACTION WITH OP_RETURN METADATA

### Command line:

```bash
python send_bwscoin.py <address> <amount> <metadata> <testnet (optional)>
Where:
  <address>  - BWS Coin address of the recipient
  <amount>   - amount to send (in units of BWS Coin)
  <metadata> - hex string or raw string containing the OP_RETURN metadata (auto-detection: treated as a hex string if it is a valid one)
  <testnet>  - BWS Coin testnet network flag (False if omitted)
```

* Outputs an error if one occurred or the txid if sending was successful


#### Examples:
```bash
python send_bwscoin.py 149wHUMa41Xm2jnZtqgRx94uGbZD9kPXnS 0.001 "Hello, myBWS!"
python send_bwscoin.py 149wHUMa41Xm2jnZtqgRx94uGbZD9kPXnS 0.001 48656c6c6f2c206d7950414921
python send_bwscoin.py mzEJxCrdva57shpv62udriBBgMECmaPce4 0.001 "Hello, BWS Coin testnet!" 1
```

### API:

**Transaction.bwscoin_send**(*address, amount, metadata, testnet=False*)<br>
`address` - BWS Coin address of the recipient<br>
`amount` - amount to send (in units of BWS Coin)<br>
`metadata` - string of raw bytes containing OP_RETURN metadata<br>
`testnet` - BWS Coin testnet network flag (False if omitted)<br>

**Return:**<br>
{'`BWScoinTransactionError`': 'error string'}<br>
or:<br>
{'txid': 'sent txid'}

#### Examples:

```python
from bwscointxn import Transaction

Transaction.bwscoin_send('149wHUMa41Xm2jnZtqgRx94uGbZD9kPXnS', 0.001, 'Hello, myBWS!')
Transaction.bwscoin_send('mzEJxCrdva57shpv62udriBBgMECmaPce4', 0.001, 'Hello, BWS Coin testnet!', testnet=True)

# Equivalent with:

bwscoin_txn = Transaction(testnet=True)
res = bwscoin_txn.send('mzEJxCrdva57shpv62udriBBgMECmaPce4', 0.001, 'Hello, BWS Coin testnet!')
```

## STORE DATA IN THE BLOCKCHAIN USING OP_RETURN

### Command line:
```bash
python store_bwscoin.py <data> <testnet (optional)>
Where:
  <data>    - hex string or raw string containing the data to be stored (auto-detection: treated as a hex string if it is a valid one)
  <testnet> - BWS Coin testnet network flag (False if omitted)

```
* Outputs an error if one occurred or if successful, the txids that were used to store
  the data and a short reference that can be used to retrieve it using this library.

#### Examples:
```bash
python store_bwscoin.py "This example stores 47 bytes in the blockchain."
python store_bitcoin.py "This example stores 44 bytes in the testnet." 1
```
  
### API:

**Transaction.bwscoin_store**(*data, testnet=False*)<br>
`data` -  string of raw bytes containing OP_RETURN metadata<br>
`testnet` - BWS Coin testnet network flag (False if omitted)<br>
  
**Return:**<br>
{'`BWScoinTransactionError`': 'error string'}<br>
or:<br>
{'txid': 'sent txid', 'ref': 'ref for retrieving data' }
           
#### Examples:

```python
from bwscointxn import Transaction

Transaction.bwscoin_store('This example stores 47 bytes in the blockchain.')
Transaction.bwscoin_store('This example stores 44 bytes in the testnet.', testnet=True)

# Equivalent with:

bwscoin_txn = Transaction(testnet=True)
res = bwscoin_txn.store('This example stores 44 bytes in the testnet.')
```

## RETRIEVE DATA FROM OP_RETURN IN THE BLOCKCHAIN

### Command line:

```bash
python retrieve_bwscoin.py <ref> <testnet (optional)>
Where:
  <ref>     - reference that was returned by a previous storage operation
  <testnet> - BWS Coin testnet network flag (False if omitted)
```

* Outputs an error if one occurred or if successful, the retrieved data in hexadecimal
  and ASCII format, a list of the txids used to store the data, a list of the blocks in
  which the data is stored, and (if available) the best ref for retrieving the data
  quickly in future. This may or may not be different from the ref you provided.
  
#### Examples:

```bash
python retrieve_bwscoin.py 356115-052075
python retrieve_bwscoin.py 396381-059737 1
```
  
### API:

**Transaction.bwscoin_retrieve**(*ref, max_results=1, testnet=False*)<br>
`ref` - reference that was returned by a previous storage operation<br>
`max_results` - maximum number of results to retrieve (omit for 1)<br>
`testnet` - BWS Coin testnet network flag (False if omitted)<br>

**Returns**:<br>
{'`BWScoinTransactionError`': 'error string'}<br>
or:<br>
{'data': 'raw binary data',<br>
 'txids': ['1st txid', '2nd txid', ...],<br>
 'heights': [block 1 used, block 2 used, ...],<br>
 'ref': 'best ref for retrieving data',<br>
 'error': 'error if data only partially retrieved'}

**NOTE:**<br>
A value of 0 in the 'heights' array means that data is still in the mempool.<br>
The 'ref' and 'error' elements are only present if appropriate.
                 
#### Examples:
```python
from bwscointxn import Transaction

Transaction.bwscoin_retrieve('356115-052075')
Transaction.bwscoin_retrieve('396381-059737', 1, True)

# Equivalent with:

bwscoin_txn = Transaction(testnet=True)
res = bwscoin_txn.retrieve('396381-059737', max_results=1)
```

##### VERSION HISTORY

v2.2.0 - 5 February 2018
