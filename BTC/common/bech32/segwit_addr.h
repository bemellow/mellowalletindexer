/* Copyright (c) 2017 Pieter Wuille
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <libmisc/declspec.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace segwit_addr
{

/** Decode a SegWit address. Returns (witver, witprog). witver = -1 means failure. */
LIBMISC_API std::pair<int, std::vector<uint8_t> > decode(const std::string& hrp, const std::string& addr);

/** Encode a SegWit address. Empty string means failure. */
LIBMISC_API std::string encode(const char *hrp, int witver, const std::vector<uint8_t> &witprog);
LIBMISC_API std::string encode(const char *hrp, int witver, const void *witprog, size_t witprog_size);

inline std::string encode(const void *witprog, size_t witprog_size){
	return encode("bc", 0, witprog, witprog_size);
}

inline std::string encode(const std::vector<uint8_t> &witprog){
	return encode("bc", 0, witprog);
}

}
