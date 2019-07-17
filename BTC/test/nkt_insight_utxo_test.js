/* global Promise */

const request = require('request');
const assert = require('assert');

async function insight(addr) {
    return new Promise((resolve, reject) => {
        console.log("INSIGHT call", addr);
        request('https://insight.bitpay.com/api/addrs/' + addr + "/utxo",
                function (error, response, body) {
                    if (error) {
                        reject(error);
                    } else {
                        let id = JSON.parse(body);
                        //console.log("INSIGHT ret", id);
                        resolve(id);
                    }
                });
    });
}

async function nkt(addr) {
    return new Promise((resolve, reject) => {
        console.log("NKT call", addr);
        request({url: 'http://gethfull:8080/api/utxo',
            method: "POST",
            json: [addr]
        }, function (error, response, body) {
            if (error) {
                reject(error);
            } else {
                //console.log("NKT ret", body);
                resolve(body);
            }
        });
    });
}


async function checkAddr(addr) {
    let data = await Promise.all([nkt(addr), insight(addr)]);
    let nktData = data[0];
    let iData = data[1];
    // insight issue not ours.
    if (iData.length === 0) {
        return;
    }
    assert.equal(nktData.length, iData.length);
    let nktMap = nktData.reduce((nktAcc, nktCur) => {
        let data = nktCur;
        if (!nktAcc[nktCur.txid]) {
            nktAcc[nktCur.txid] = {};
        }
        nktAcc[nktCur.txid][nktCur.vout] = data;
        return nktAcc;
    }, {});
    //console.log("nktnktnktnktnktnktnktnktnkt->", JSON.stringify(nktMap, null, 4));
    for (let k in iData) {
        let d = iData[k];
        assert(d.txid in nktMap);
        assert(d.vout in nktMap[d.txid]);
        assert.equal(d.address, nktMap[d.txid][d.vout].address);
        assert.equal(d.satoshis, nktMap[d.txid][d.vout].satoshis);
    }
}

async function main() {
    await checkAddr('1Cb4X74MUitDDCdkhdRRcB8eutpV5bAhdG');
    await checkAddr('12uMFsPmP7VJmkQFxps75VyLG2AwAyPnfb');
    await checkAddr('1GssKhyeGwZF8CEL1ZVS2YyfFJeEr9ZqEt');
    //await checkAddr('1dice8EMZmqKvrGE4Qc9bUFf9PX3xaYDp');
}

main().then(() => console.log("done"));
