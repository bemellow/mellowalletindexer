#include "TxOutput.h"
#include "Block.h"
#include <common/misc.h>
#include <common/serialization.h>
#include <libhash/hash.h>
#include <sstream>
#include <iomanip>

TxOutput::TxOutput(SerializedBuffer &buffer, Transaction &parent, bool testnet)
		: parent(&parent)
		, testnet(testnet){
	this->value = buffer.read_u64();
	this->script = buffer.read_sized_buffer();
}

enum opcodetype{
	// push value
	OP_0 = 0x00,
	OP_FALSE = OP_0,
	OP_PUSHDATA1 = 0x4c,
	OP_PUSHDATA2 = 0x4d,
	OP_PUSHDATA4 = 0x4e,
	OP_1NEGATE = 0x4f,
	OP_RESERVED = 0x50,
	OP_1 = 0x51,
	OP_TRUE = OP_1,
	OP_2 = 0x52,
	OP_3 = 0x53,
	OP_4 = 0x54,
	OP_5 = 0x55,
	OP_6 = 0x56,
	OP_7 = 0x57,
	OP_8 = 0x58,
	OP_9 = 0x59,
	OP_10 = 0x5a,
	OP_11 = 0x5b,
	OP_12 = 0x5c,
	OP_13 = 0x5d,
	OP_14 = 0x5e,
	OP_15 = 0x5f,
	OP_16 = 0x60,

	// control
	OP_NOP = 0x61,
	OP_VER = 0x62,
	OP_IF = 0x63,
	OP_NOTIF = 0x64,
	OP_VERIF = 0x65,
	OP_VERNOTIF = 0x66,
	OP_ELSE = 0x67,
	OP_ENDIF = 0x68,
	OP_VERIFY = 0x69,
	OP_RETURN = 0x6a,

	// stack ops
	OP_TOALTSTACK = 0x6b,
	OP_FROMALTSTACK = 0x6c,
	OP_2DROP = 0x6d,
	OP_2DUP = 0x6e,
	OP_3DUP = 0x6f,
	OP_2OVER = 0x70,
	OP_2ROT = 0x71,
	OP_2SWAP = 0x72,
	OP_IFDUP = 0x73,
	OP_DEPTH = 0x74,
	OP_DROP = 0x75,
	OP_DUP = 0x76,
	OP_NIP = 0x77,
	OP_OVER = 0x78,
	OP_PICK = 0x79,
	OP_ROLL = 0x7a,
	OP_ROT = 0x7b,
	OP_SWAP = 0x7c,
	OP_TUCK = 0x7d,

	// splice ops
	OP_CAT = 0x7e,
	OP_SUBSTR = 0x7f,
	OP_LEFT = 0x80,
	OP_RIGHT = 0x81,
	OP_SIZE = 0x82,

	// bit logic
	OP_INVERT = 0x83,
	OP_AND = 0x84,
	OP_OR = 0x85,
	OP_XOR = 0x86,
	OP_EQUAL = 0x87,
	OP_EQUALVERIFY = 0x88,
	OP_RESERVED1 = 0x89,
	OP_RESERVED2 = 0x8a,

	// numeric
	OP_1ADD = 0x8b,
	OP_1SUB = 0x8c,
	OP_2MUL = 0x8d,
	OP_2DIV = 0x8e,
	OP_NEGATE = 0x8f,
	OP_ABS = 0x90,
	OP_NOT = 0x91,
	OP_0NOTEQUAL = 0x92,

	OP_ADD = 0x93,
	OP_SUB = 0x94,
	OP_MUL = 0x95,
	OP_DIV = 0x96,
	OP_MOD = 0x97,
	OP_LSHIFT = 0x98,
	OP_RSHIFT = 0x99,

	OP_BOOLAND = 0x9a,
	OP_BOOLOR = 0x9b,
	OP_NUMEQUAL = 0x9c,
	OP_NUMEQUALVERIFY = 0x9d,
	OP_NUMNOTEQUAL = 0x9e,
	OP_LESSTHAN = 0x9f,
	OP_GREATERTHAN = 0xa0,
	OP_LESSTHANOREQUAL = 0xa1,
	OP_GREATERTHANOREQUAL = 0xa2,
	OP_MIN = 0xa3,
	OP_MAX = 0xa4,

	OP_WITHIN = 0xa5,

	// crypto
	OP_RIPEMD160 = 0xa6,
	OP_SHA1 = 0xa7,
	OP_SHA256 = 0xa8,
	OP_HASH160 = 0xa9,
	OP_HASH256 = 0xaa,
	OP_CODESEPARATOR = 0xab,
	OP_CHECKSIG = 0xac,
	OP_CHECKSIGVERIFY = 0xad,
	OP_CHECKMULTISIG = 0xae,
	OP_CHECKMULTISIGVERIFY = 0xaf,

	// expansion
	OP_NOP1 = 0xb0,
	OP_CHECKLOCKTIMEVERIFY = 0xb1,
	OP_NOP2 = OP_CHECKLOCKTIMEVERIFY,
	OP_CHECKSEQUENCEVERIFY = 0xb2,
	OP_NOP3 = OP_CHECKSEQUENCEVERIFY,
	OP_NOP4 = 0xb3,
	OP_NOP5 = 0xb4,
	OP_NOP6 = 0xb5,
	OP_NOP7 = 0xb6,
	OP_NOP8 = 0xb7,
	OP_NOP9 = 0xb8,
	OP_NOP10 = 0xb9,

	OP_INVALIDOPCODE = 0xff,
};
#define PUSHBYTES(n) n
#define IS_PUSHBYTES(n) ((n) < OP_PUSHDATA1)
#define IS_PUSHBYTES_N(n, m) (IS_PUSHBYTES(n) && (n) == (m))

#define CANT_DECODE_ADDRESS { this->address_type = AddressType::Unknown; break; }

struct SimplifiedInstruction{
	int opcode;
	u32 parameter;
	size_t offset;
	const void *data;
	bool operator<(const SimplifiedInstruction &other) const{
		if (this->opcode < other.opcode)
			return true;
		if (this->opcode > other.opcode)
			return false;
		return this->parameter < other.parameter;
	}
	void copy(void *dst){
		memcpy(dst, this->data, this->parameter);
	}
	u32 read_integer() const{
		if (this->opcode != OP_PUSHDATA1 || this->parameter < 1 || this->parameter > 4)
			return std::numeric_limits<u32>::max();
		u32 ret = 0;
		auto p = (const u8 *)this->data + this->offset;
		for (size_t i = this->parameter; i--;){
			ret <<= 8;
			ret |= p[i];
		}
		return ret;
	}
};

typedef std::deque<SimplifiedInstruction> simplified_script;

simplified_script simplify_script(const unsigned char *s, size_t n){
	simplified_script ret;
	for (size_t i = 0; i < n;){
		SimplifiedInstruction si;
		si.offset = i;
		si.data = nullptr;
		auto byte = s[i++];
		if (byte >= 1 && byte <= 75){
			si.opcode = OP_PUSHDATA1;
			si.parameter = byte;
		} else{
			si.opcode = byte;
			si.parameter = 0;
			switch (byte){
			case OP_PUSHDATA1:
				if (i + 1 > n)
					return{};
				si.parameter = s[i++];

				break;
			case OP_PUSHDATA2:
				if (i + 2 > n)
					return{};
				si.opcode = OP_PUSHDATA1;
				si.parameter = s[i++];
				si.parameter |= s[i++] << 8;
				break;
			case OP_PUSHDATA4:
				if (i + 4 > n)
					return{};
				si.opcode = OP_PUSHDATA1;
				si.parameter = s[i++];
				si.parameter |= s[i++] << 8;
				si.parameter |= s[i++] << 16;
				si.parameter |= s[i++] << 24;
				break;
			}
		}
		if (si.opcode == OP_PUSHDATA1){
			if (i + si.parameter > n)
				return{};
			si.data = s + i;
			i += si.parameter;
		}
		ret.push_back(si);
	}
	return ret;
}

struct simplified_script_cmp{
	bool operator()(const simplified_script &a, const simplified_script &b){
		size_t min = std::min(a.size(), b.size());
		for (size_t i = 0; i < min; i++){
			if (a[i] < b[i])
				return true;
			if (b[i] < a[i])
				return false;
		}
		return a.size() < b.size();
	}
};

#define N nullptr
static const char * const opcode_strings[] = {
	"OP_0", N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
	N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
	N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
	N, N, "OP_PUSHDATA1", N, N, "OP_1NEGATE", "OP_RESERVED", "OP_1", "OP_2",
	"OP_3", "OP_4", "OP_5", "OP_6", "OP_7", "OP_8", "OP_9", "OP_10", "OP_11",
	"OP_12", "OP_13", "OP_14", "OP_15", "OP_16", "OP_NOP", "OP_VER", "OP_IF",
	"OP_NOTIF", "OP_VERIF", "OP_VERNOTIF", "OP_ELSE", "OP_ENDIF", "OP_VERIFY",
	"OP_RETURN", "OP_TOALTSTACK", "OP_FROMALTSTACK", "OP_2DROP", "OP_2DUP",
	"OP_3DUP", "OP_2OVER", "OP_2ROT", "OP_2SWAP", "OP_IFDUP", "OP_DEPTH",
	"OP_DROP", "OP_DUP", "OP_NIP", "OP_OVER", "OP_PICK", "OP_ROLL", "OP_ROT",
	"OP_SWAP", "OP_TUCK", "OP_CAT", "OP_SUBSTR", "OP_LEFT", "OP_RIGHT",
	"OP_SIZE", "OP_INVERT", "OP_AND", "OP_OR", "OP_XOR", "OP_EQUAL",
	"OP_EQUALVERIFY", "OP_RESERVED1", "OP_RESERVED2", "OP_1ADD", "OP_1SUB",
	"OP_2MUL", "OP_2DIV", "OP_NEGATE", "OP_ABS", "OP_NOT", "OP_0NOTEQUAL",
	"OP_ADD", "OP_SUB", "OP_MUL", "OP_DIV", "OP_MOD", "OP_LSHIFT", "OP_RSHIFT",
	"OP_BOOLAND", "OP_BOOLOR", "OP_NUMEQUAL", "OP_NUMEQUALVERIFY",
	"OP_NUMNOTEQUAL", "OP_LESSTHAN", "OP_GREATERTHAN", "OP_LESSTHANOREQUAL",
	"OP_GREATERTHANOREQUAL", "OP_MIN", "OP_MAX", "OP_WITHIN", "OP_RIPEMD160",
	"OP_SHA1", "OP_SHA256", "OP_HASH160", "OP_HASH256", "OP_CODESEPARATOR",
	"OP_CHECKSIG", "OP_CHECKSIGVERIFY", "OP_CHECKMULTISIG",
	"OP_CHECKMULTISIGVERIFY", "OP_NOP1", "OP_CHECKLOCKTIMEVERIFY",
	"OP_CHECKSEQUENCEVERIFY", "OP_NOP4", "OP_NOP5", "OP_NOP6", "OP_NOP7",
	"OP_NOP8", "OP_NOP9", "OP_NOP10", N, N, N, N, N, N, N, N, N, N, N, N, N, N,
	N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
	N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
	N, N, N, N, N, "OP_INVALIDOPCODE",
};
#undef N

std::string to_string(const simplified_script &s){
	std::stringstream stream;
	stream << '"';
	bool first = true;
	for (auto &i : s){
		if (first)
			first = false;
		else
			stream << ' ';
		if (i.opcode == OP_PUSHDATA1)
			stream << "PUSHBYTES(" << i.parameter << ")";
		else{
			auto string = opcode_strings[i.opcode];
			if (string)
				stream << string;
			else
				stream << "INVALID_OPCODE_" << i.opcode;
		}
	}
	stream << '"';
	return stream.str();
}

bool matches(const simplified_script &simplified, const std::vector<int> &xs){
	size_t i = 0;
	auto n = simplified.size();
	for (auto x : xs){
		if (x < 0)
			return i == n;
		if (i == n)
			return false;
		if (x < 256){
			if (simplified[i].opcode != x)
				return false;
		} else if (simplified[i].opcode != OP_PUSHDATA1 || simplified[i].parameter != x - 256)
			return false;
		i++;
	}
	return true;
}

#define MPUSHBYTES(n) (256 + (n))
#define EOS -1

typedef std::vector<int> script_matcher;

static const script_matcher nop1 = { OP_PUSHDATA1, OP_DROP };
static const script_matcher invalid1 = { OP_PUSHDATA1, EOS };

//P2PKH
static const script_matcher P2PKH_1 = { OP_DUP, OP_HASH160, MPUSHBYTES(20), OP_EQUALVERIFY, OP_CHECKSIG, EOS };
static const script_matcher P2PKH_2 = { OP_DUP, OP_HASH160, MPUSHBYTES(20), OP_EQUALVERIFY, OP_CHECKSIG, OP_NOP, EOS };
static const script_matcher P2PKH_3 = { OP_DUP,     OP_DUP,         OP_HASH160,     OP_DROP, OP_HASH160, MPUSHBYTES(20), OP_EQUALVERIFY, OP_CHECKSIG, EOS };
static const script_matcher P2PKH_4 = { OP_HASH256, MPUSHBYTES(32), OP_EQUALVERIFY, OP_DUP,  OP_HASH160, MPUSHBYTES(20), OP_EQUALVERIFY, OP_CHECKSIG, EOS };

//P2PK
static const script_matcher P2PK_1 = { MPUSHBYTES(33), OP_CHECKSIG, EOS };
static const script_matcher P2PK_2 = { MPUSHBYTES(65), OP_CHECKSIG, EOS };

//P2SH
static const script_matcher P2SH = { OP_HASH160, MPUSHBYTES(20), OP_EQUAL, EOS };

//P2WPKH
static const script_matcher P2WPKH_1 = { OP_FALSE, MPUSHBYTES(20), EOS };
static const script_matcher P2WPKH_2 = { OP_FALSE, MPUSHBYTES(32), EOS };

//Malformed P2PKH:
//static const script_matcher invalid = {OP_DUP, OP_HASH160, MPUSHBYTES(20), OP_EQUAL, OP_CHECKSIG, EOS};

//static const script_matcher cant_spend1  = {OP_DUP, OP_HASH160, MPUSHBYTES(20), OP_EQUALVERIFY, OP_CHECKSIG, OP_FALSE, EOS};
//static const script_matcher cant_spend2  = {OP_SHA256, MPUSHBYTES(29), PUSHBYTES(37), IGNORE(3), EOS};

static const script_matcher unconfirmed1 = { OP_PUSHDATA1, OP_CHECKSIG, EOS };

bool is_multisig(int &minimum, int &maximum, const simplified_script &s){
	if (s.size() < 4 || s.back().opcode != OP_CHECKMULTISIG)
		return false;
	auto max_op = s[s.size() - 2].opcode;
	if (max_op == OP_PUSHDATA1){
		maximum = s[s.size() - 2].read_integer();
		if (maximum < 1 || maximum > 20)
			return false;
	} else{
		if (max_op < OP_1 || max_op > OP_16)
			return false;
		maximum = max_op - OP_1 + 1;
	}
	if (s.size() != 3 + maximum)
		return false;
	auto min_op = s.front().opcode;
	if (min_op == OP_PUSHDATA1){
		minimum = s.front().read_integer();
		if (minimum > maximum)
			return false;
	} else{
		if ((min_op < OP_1 && min_op != OP_0) || min_op > max_op)
			return false;
		minimum = min_op == OP_0 ? 0 : (min_op - OP_1 + 1);
	}
	size_t b = 1;
	size_t e = s.size() - 2;
	for (auto i = b; i < e; i++){
		if (s[i].opcode != OP_PUSHDATA1)
			return false;
		if (s[i].parameter != 33 && s[i].parameter != 65)
			return false;
	}
	return true;
}

Hashes::Digests::SHA256 compute_sha256(const SimplifiedInstruction &si){
	return Hashes::Algorithms::SHA256::compute(si.data, si.parameter);
}

void TxOutput::compute_output_addresses(){
	auto simplified = simplify_script(&this->script[0], this->script.size());
	while (simplified.size()){
		if (simplified.front().opcode == OP_RETURN || matches(simplified, invalid1)){
			this->address_type = AddressType::Unspendable;
			break;
		}

		if (matches(simplified, nop1)){
			simplified.pop_front();
			simplified.pop_front();
			continue;
		}

		{
			int p2pkh_pick_from = -1;
			if (matches(simplified, P2PKH_1) || matches(simplified, P2PKH_2))
				p2pkh_pick_from = 2;
			else if (matches(simplified, P2PKH_3) || matches(simplified, P2PKH_4))
				p2pkh_pick_from = 5;
			if (p2pkh_pick_from >= 0){
				this->address_type = AddressType::P2pkh;
				full_assert(Hashes::Digests::RIPEMD160::size == simplified[p2pkh_pick_from].parameter);
				this->addresses.emplace_back(this->address_type, simplified[p2pkh_pick_from].data, this->testnet);
				break;
			}
		}

		if (matches(simplified, P2PK_1) || matches(simplified, P2PK_2)){
			this->address_type = AddressType::P2pk;
			auto hash = Hashes::Algorithms::RIPEMD160::compute(compute_sha256(simplified.front()).to_array()).to_array();
			this->addresses.emplace_back(this->address_type, hash.data(), this->testnet);
			break;
		}

		if (matches(simplified, P2SH)){
			this->address_type = AddressType::P2sh;
			assert(Hashes::Digests::RIPEMD160::size == simplified[1].parameter);
			this->addresses.emplace_back(this->address_type, simplified[1].data, this->testnet);
			break;
		}

		if (matches(simplified, P2WPKH_1)){
			this->address_type = AddressType::P2wpkh20;
		} else if (matches(simplified, P2WPKH_2)){
			this->address_type = AddressType::P2wpkh32;
		}

		if (this->address_type == AddressType::P2wpkh20 || this->address_type == AddressType::P2wpkh32){
			auto &i = simplified[1];
			this->addresses.emplace_back(this->address_type, i.data, this->testnet);
			break;
		}

		int minimum_signatures, maximum_signatures;
		if (is_multisig(minimum_signatures, maximum_signatures, simplified)){
			this->address_type = AddressType::P2pkh;
			for (size_t i = 0; i < maximum_signatures; i++){
				auto &ins = simplified[i + 1];
				assert(ins.opcode == OP_PUSHDATA1 && (ins.parameter == 33 || ins.parameter == 65));
				auto hash = Hashes::Algorithms::RIPEMD160::compute(compute_sha256(ins).to_array()).to_array();
				this->addresses.emplace_back(this->address_type, hash.data(), this->testnet);
			}
			this->required_spenders = minimum_signatures;
			break;
		}

#ifdef FIND_UNKNOWN_SCRIPTS
		if (dedupper)
			dedupper->add(simplified, this->parent->get_hash());
#endif

		CANT_DECODE_ADDRESS;

		std::stringstream stream;
		stream << std::hex << "Can't compute output address:";
		for (auto b : this->script)
			stream << ' ' << std::setw(2) << std::setfill('0') << (int)b;

		throw std::runtime_error(stream.str());

		return;
	}

	this->script.clear();
	this->script.shrink_to_fit();
}

std::set<u64> TxOutput::insert(u64 txid, u32 txo_index, InsertState &nis){
	this->compute_output_addresses();
	auto txoid = nis.insert_output(txid, txo_index, this->value, this->required_spenders, this->script);
	std::set<u64> addresses;
	for (auto &addr : this->addresses)
		addresses.insert(nis.insert_address_if_it_doesnt_exist(addr));
	nis.add_addresses_outputs_relations(txoid, addresses);
	return addresses;
}
