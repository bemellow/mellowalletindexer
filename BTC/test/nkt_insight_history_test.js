/* global Promise */

const request = require('request');
const assert = require('assert');

async function insight(addr, from, to) {
    return new Promise((resolve, reject) => {
        console.log("INSIGHT call", addr, " from ", from, " to ", to);
        request('https://insight.bitpay.com/api/addrs/' + addr + "/txs?from=" + from + "&to=" + to,
                function (error, response, body) {
                    if (error) {
                        reject(error);
                    } else {
                        let id = JSON.parse(body);
                        console.log("INSIGHT ret", addr, " total ", id.totalItems, " from ", id.from, " to ", id.to);
                        resolve(id);
                    }
                });
    });
}

async function nkt(addr) {
    return new Promise((resolve, reject) => {
        console.log("NKT call", addr);
        request({url: 'http://gethfull:8080/api/history',
            method: "POST",
            json: {addresses: [addr], max_txs: 10000} // Number.MAX_SAFE_INTEGER can be problematic
        }, function (error, response, body) {
            if (error) {
                reject(error);
            } else {
                console.log("NKT ret", addr);
                resolve(body);
            }
        });
    });
}


function checkData(iData, nktMap) {
    let items = iData.items;
    for (let k in items) {
        let hash = items[k].txid;
        let nkt = nktMap[hash];
        let item = items[k];
        try {
            assert.equal(nkt.hash, item.txid);
            assert.equal(nkt.block_hash, item.blockhash);
            assert.equal(nkt.block_height, item.blockheight);
            assert.equal(nkt.blocktime, item.timestamp);
            assert.equal(nkt.block_height, item.blockheight);

            // Inputs
            let cant = 0;
            for (let io in item.vin) {
                let search = item.vin[io];
                if (search.coinbase) {
                    //skip those
                    continue;
                }
                cant++;
                let found = nkt.iMap[search.n];
                assert(found);
                assert(found.addresses.some((e) => (e === search.addr)));
                assert.equal(search.value, found.value / 100000000);
            }
            assert.equal(nkt.inputs.length, cant);

            // Outputs
            assert.equal(nkt.outputs.length, item.vout.length);
            for (let io in item.vout) {
                let search = item.vout[io];
                let found = nkt.oMap[search.n];
                assert(found);
                assert.equal(search.value, found.value / 100000000);
                let as = (search.scriptPubKey.addresses || []).sort();
                assert(as.every((u, i) => (u === found.addresses[i])));
            }

        } catch (err) {
            console.log("-------H>", k, hash);
            console.log("-------N>", JSON.stringify(nkt, null, 4));
            console.log("-------i>", JSON.stringify(item, null, 4));
            throw err;
        }
    }
}


async function checkAddr(addr) {
    let data = await Promise.all([nkt(addr), insight(addr, 0, 0)]);
    let nktData = data[0];
    let iData = data[1];
    if (iData.totalItems == 0) {
        return;
    }
    assert.equal(nktData.length, iData.totalItems);

    let nktMap = nktData.reduce((nktAcc, nktCur) => {
        let data = nktCur;
        data.iMap = data.inputs.reduce((acc, cur) => {
            acc[cur.txi_index] = cur;
            return acc;
        }, {});
        data.oMap = data.outputs.reduce((acc, cur) => {
            acc[cur.txo_index] = cur;
            return acc;
        }, {});
        nktAcc[nktCur.hash] = data;
        return nktAcc;
    }, {});
    //console.log("nktnktnktnktnktnktnktnktnkt->", JSON.stringify(nktMap, null, 4));
    let ps = [];
    for (let r = 0; r < iData.totalItems; r += 50) {
        ps.push(insight(addr, r, r + 50));
    }
    let rets = await Promise.all(ps);
    for (let p in rets) {
        checkData(rets[p], nktMap);
    }
}

async function main() {
    await checkAddr('1Cb4X74MUitDDCdkhdRRcB8eutpV5bAhdG');
    await checkAddr('12uMFsPmP7VJmkQFxps75VyLG2AwAyPnfb');
    await checkAddr('1GssKhyeGwZF8CEL1ZVS2YyfFJeEr9ZqEt');
    //await checkAddr('1dice8EMZmqKvrGE4Qc9bUFf9PX3xaYDp');
}

main().then(() => console.log("done"));
