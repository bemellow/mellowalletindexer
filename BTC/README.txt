To build on Linux
-----------------

Prerequisites:

* mono-complete package
* Usual C/++ build tools
* Download the latest version of nuget and put it somewhere in PATH

Build instructions:

cmake .
make
nuget restore
xbuild /p:Platform=x64 /p:Configuration=Release


blockchain_parser
-----------------

Usage:
blockchain_parser config_dir output_dir [<testnet>]

config_dir is the path of the directory where bitcoin.conf is. It's assumed that
the block files are at config_dir/blocks
output_dir is the path where the output database will be written.
testnet is an optional number. It tells the program whether to use testnet or
livenet formats for addresses. If omitted, it defaults to 0.

The process can be momentarily stopped with Ctrl+C. The next time it's run with
the same arguments it will resume from where it stopped.
After it finishes, the following statements should be run on the DB from within
sqlite3:

create index inputs_by_previous_tx_id on inputs(previous_tx_id);
create index inputs_by_txs_id on inputs(txs_id);
create index inputs_by_outputs_id on inputs(outputs_id);
create index addresses_outputs_by_addresses_id on addresses_outputs (addresses_id);
create index addresses_txs_by_addresses_id on addresses_txs (addresses_id);
create index addresses_txs_by_txs_id on addresses_txs (txs_id);


NktBtcIndex Configuration
-------------------------

BtcIndex.conf
{
    "hostname": string,     // Hostname where bitcoind is running (default: "localhost")
    "rpc_port": number,     // Port where bitcoind is listening for RPC connections (default: 8332)
    "testnet": bool,        // Which network to use? (default: false)
    "rpc_user": string,     // User name for RPC authentication. Required.
    "rpc_password": string, // Password for RPC authentication. Required.
    "db_path": string,      // Path to the SQLite database file. Required.
}
