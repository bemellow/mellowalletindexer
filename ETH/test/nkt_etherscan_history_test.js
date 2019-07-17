/* global Promise */

const request = require('request');
const assert = require('assert');

async function etherscanInt(addr, page) {
    return new Promise((resolve, reject) => {
        console.log("ETHERSCAN call", addr, page);
        let apiKey = "UJR2SPTG11SCCA4GEQD6YQQX6UX52FFTKA";
        request("https://api-ropsten.etherscan.io/api"
                + "?apiKey=" + apiKey
                + "&module=account"
                + "&action=txlist"
                + "&address=" + addr
                + "&sort=asc"
                + "&page=" + page
                + "&offset=5000",
                function (error, response, body) {
                    if (error) {
                        reject(error);
                    } else {
                        let id = JSON.parse(body);
                        console.log("ETHERSCAN ret", addr, id.message, id.result.length);
                        resolve(id.result);
                    }
                });
    });
}

async function etherscan(addr) {
    let ret = [];
    for (let page = 1; ; page++) {
        let x = await etherscanInt(addr, page);
        if (x.length === 0) {
            break;
        }
        ret = ret.concat(x);
    }
    return ret;
}

async function nkt(addr) {
    return new Promise((resolve, reject) => {
        console.log("NKT call", addr);
        request({url: 'http://gethfull:8083/api/history',
            method: "POST",
            json: {addresses: [addr], max_txs: 10000000}
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


async function checkAddr(addr) {
    let data = await Promise.all([nkt(addr), etherscan(addr)]);
    let nktData = data[0];
    let iData = data[1];

    assert.equal(nktData.length, iData.length);
    //console.log("NTK-------------------->", nktData);
    //console.log("SCA-------------------->", iData);
    let nktMap = nktData.reduce((nktAcc, nktCur) => {
        let data = nktCur;
        nktAcc[nktCur.txId] = data;
        return nktAcc;
    }, {});
    //console.log("nktnktnktnktnktnktnktnktnkt->", JSON.stringify(nktMap, null, 4));
    for (let k in iData) {
        let item = iData[k];
        let hash = item.hash;
        let nkt = nktMap[hash];
        try {
            assert.equal(nkt.txId, item.hash);
            assert.equal(nkt.blockHash, item.blockHash);
            assert.equal(nkt.blockNumber, item.blockNumber);
            assert.equal(nkt.timestamp, item.timeStamp);
            assert.equal(nkt.input, item.from);
            if (item.to !== "") {
                assert.equal(nkt.output, item.to);
            }
            assert.equal(nkt.value, item.value);
        } catch (err) {
            console.log("-------H>", k, hash);
            console.log("-------N>", JSON.stringify(nkt, null, 4));
            console.log("-------i>", JSON.stringify(item, null, 4));
            throw err;
        }
    }
}

async function main() {
    await checkAddr("0x4841Af3c3e11184Ed79d15C4A69Ba28342D4A6d6");
    await checkAddr("0x2742123F3E3166f8fE2C0018A1e27aF037DaB718");
}

main().then(() => console.log("done"));
