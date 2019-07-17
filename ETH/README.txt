To build on Linux
-----------------

Prerequisites:

* mono-complete package
* Usual C/++ build tools
* Download the latest version of nuget and put it somewhere in PATH

Build instructions:

nuget restore
xbuild /p:Platform=x64 /p:Configuration=Release
cd bigint_cache
./build_linux.sh
cp libcpphelper.so ../bin64/

Note: SQLite for .NET may not include the required interop library. If that's
the case, extract sqlite-netFx-source-1.0.109.0.tar.xz and do

cd sqlite-netFx-source-1.0.109.0/Setup
chmod +x compile-interop-assembly-release.sh
./compile-interop-assembly-release.sh
cd ../bin/2013/Release/bin
cp * <root of ethereum_index>/bin64


If initial processing is too slow, you can try momentarily deleting these
indices from the DB:

drop index bigints_by_value;
drop index txs_by_sender;
drop index txs_by_receiver;

These are only necessary to answer requests, not during the initial processing.
Once the index is ready to answer requests, you can rebuild the indices with

create index bigints_by_value on bigints(value);
create index txs_by_sender on txs (sender);
create index txs_by_receiver on txs (receiver);


EthereumIndex configuration
---------------------------

EthIndex.conf
{
    "hostname": string,       // Hostname where Geth is running (default: "localhost")
    "rpc_port": number,       // Port where Geth is listening for RPC connections (default: 8545)
    "db_path": string,        // Path to the SQLite database file. Required.
    "inputs_path": string,    // Path to the binary store of tx inputs. Required.
    "logs_path": string,      // Path to the binary store of tx logs. Required.
    "binary_buffers": bool,   // Read all inputs and logs. Do not set. (default: false)
}
