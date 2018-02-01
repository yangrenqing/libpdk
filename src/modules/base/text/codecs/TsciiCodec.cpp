// @copyright 2017-2018 zzu_softboy <zzu_softboy@163.com>
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Created by softboy on 2018/02/01.

#include "pdk/base/text/codecs/internal/TsciiCodecPrivate.h"
#include <list>

namespace pdk {
namespace text {
namespace codecs {
namespace internal {

using pdk::lang::Latin1Character;

namespace {

static unsigned char pdk_unicode_to_tscii(ushort u1, ushort u2, ushort u3);
static unsigned int pdk_tscii_to_unicode(unsigned int code, uint *s);


} // anonymous namespace


#define IS_TSCII_CHAR(c)        (((c) >= 0x80) && ((c) <= 0xfd))


TsciiCodec::~TsciiCodec()
{
}

ByteArray TsciiCodec::convertFromUnicode(const Character *uc, int len, ConverterState *state) const
{
   char replacement = '?';
   if (state) {
      if (state->m_flags & ConversionFlag::ConvertInvalidToNull) {
         replacement = 0;
      }
   }
   int invalid = 0;
   
   ByteArray rstr(len, pdk::Uninitialized);
   uchar* cursor = (uchar*)rstr.getRawData();
   for (int i = 0; i < len; i++) {
      Character ch = uc[i];
      uchar j;
      if (ch.getRow() == 0x00 && ch.getCell() < 0x80) {
         // ASCII
         j = ch.getCell();
      } else if ((j = pdk_unicode_to_tscii(uc[i].unicode(),
                                           uc[i + 1].unicode(),
                                           uc[i + 2].unicode()))) {
         // We have to check the combined chars first!
         i += 2;
      } else if ((j = pdk_unicode_to_tscii(uc[i].unicode(),
                                           uc[i + 1].unicode(), 0))) {
         i++;
      } else if ((j = pdk_unicode_to_tscii(uc[i].unicode(), 0, 0))) {
      } else {
         // Error
         j = replacement;
         ++invalid;
      }
      *cursor++ = j;
   }
   rstr.resize(cursor - (const uchar*)rstr.getConstRawData());
   
   if (state) {
      state->m_invalidChars += invalid;
   }
   return rstr;
}


String TsciiCodec::convertToUnicode(const char* chars, int len, ConverterState *state) const
{
   Character replacement = Character::ReplacementCharacter;
   if (state) {
      if (state->m_flags & ConversionFlag::ConvertInvalidToNull)
         replacement = Character::Null;
   }
   int invalid = 0;
   
   String result;
   for (int i = 0; i < len; i++) {
      uchar ch = chars[i];
      if (ch < 0x80) {
         // ASCII
         result += Latin1Character(ch);
      } else if (IS_TSCII_CHAR(ch)) {
         // TSCII
         uint s[3];
         uint u = pdk_tscii_to_unicode(ch, s);
         uint *p = s;
         while (u--) {
            uint c = *p++;
            if (c)
               result += Character(c);
            else {
               result += replacement;
               ++invalid;
            }
         }
      } else {
         // Invalid
         result += replacement;
         ++invalid;
      }
   }
   
   if (state) {
      state->m_invalidChars += invalid;
   }
   return result;
}

ByteArray TsciiCodec::name() const
{
   return "TSCII";
}

int TsciiCodec::mibEnum() const
{
   return 2107;
}

static const int sg_UnToTsLast = 124; // 125 items -- so the last will be 124
static const ushort sg_UnToTs [][4] = {
   // *Sorted* list of TSCII maping for unicode chars
   //FIRST  SECOND  THIRD   TSCII
   {0x00A0, 0x0000, 0x0000, 0xA0},
   {0x00A9, 0x0000, 0x0000, 0xA9},
   {0x0B83, 0x0000, 0x0000, 0xB7},
   {0x0B85, 0x0000, 0x0000, 0xAB},
   {0x0B86, 0x0000, 0x0000, 0xAC},
   {0x0B87, 0x0000, 0x0000, 0xAD},
   {0x0B88, 0x0000, 0x0000, 0xAE},
   {0x0B89, 0x0000, 0x0000, 0xAF},
   {0x0B8A, 0x0000, 0x0000, 0xB0},
   {0x0B8E, 0x0000, 0x0000, 0xB1},
   {0x0B8F, 0x0000, 0x0000, 0xB2},
   {0x0B90, 0x0000, 0x0000, 0xB3},
   {0x0B92, 0x0000, 0x0000, 0xB4},
   {0x0B93, 0x0000, 0x0000, 0xB5},
   {0x0B94, 0x0000, 0x0000, 0xB6},
   {0x0B95, 0x0000, 0x0000, 0xB8},
   {0x0B95, 0x0B82, 0x0000, 0xEC},
   {0x0B95, 0x0BC1, 0x0000, 0xCC},
   {0x0B95, 0x0BC2, 0x0000, 0xDC},
   {0x0B99, 0x0000, 0x0000, 0xB9},
   {0x0B99, 0x0B82, 0x0000, 0xED},
   {0x0B99, 0x0BC1, 0x0000, 0x99},
   {0x0B99, 0x0BC2, 0x0000, 0x9B},
   {0x0B9A, 0x0000, 0x0000, 0xBA},
   {0x0B9A, 0x0B82, 0x0000, 0xEE},
   {0x0B9A, 0x0BC1, 0x0000, 0xCD},
   {0x0B9A, 0x0BC2, 0x0000, 0xDD},
   {0x0B9C, 0x0000, 0x0000, 0x83},
   {0x0B9C, 0x0B82, 0x0000, 0x88},
   {0x0B9E, 0x0000, 0x0000, 0xBB},
   {0x0B9E, 0x0B82, 0x0000, 0xEF},
   {0x0B9E, 0x0BC1, 0x0000, 0x9A},
   {0x0B9E, 0x0BC2, 0x0000, 0x9C},
   {0x0B9F, 0x0000, 0x0000, 0xBC},
   {0x0B9F, 0x0B82, 0x0000, 0xF0},
   {0x0B9F, 0x0BBF, 0x0000, 0xCA},
   {0x0B9F, 0x0BC0, 0x0000, 0xCB},
   {0x0B9F, 0x0BC1, 0x0000, 0xCE},
   {0x0B9F, 0x0BC2, 0x0000, 0xDE},
   {0x0BA1, 0x0B82, 0x0000, 0xF2},
   {0x0BA3, 0x0000, 0x0000, 0xBD},
   {0x0BA3, 0x0B82, 0x0000, 0xF1},
   {0x0BA3, 0x0BC1, 0x0000, 0xCF},
   {0x0BA3, 0x0BC2, 0x0000, 0xDF},
   {0x0BA4, 0x0000, 0x0000, 0xBE},
   {0x0BA4, 0x0BC1, 0x0000, 0xD0},
   {0x0BA4, 0x0BC2, 0x0000, 0xE0},
   {0x0BA8, 0x0000, 0x0000, 0xBF},
   {0x0BA8, 0x0B82, 0x0000, 0xF3},
   {0x0BA8, 0x0BC1, 0x0000, 0xD1},
   {0x0BA8, 0x0BC2, 0x0000, 0xE1},
   {0x0BA9, 0x0000, 0x0000, 0xC9},
   {0x0BA9, 0x0B82, 0x0000, 0xFD},
   {0x0BA9, 0x0BC1, 0x0000, 0xDB},
   {0x0BA9, 0x0BC2, 0x0000, 0xEB},
   {0x0BAA, 0x0000, 0x0000, 0xC0},
   {0x0BAA, 0x0B82, 0x0000, 0xF4},
   {0x0BAA, 0x0BC1, 0x0000, 0xD2},
   {0x0BAA, 0x0BC2, 0x0000, 0xE2},
   {0x0BAE, 0x0000, 0x0000, 0xC1},
   {0x0BAE, 0x0B82, 0x0000, 0xF5},
   {0x0BAE, 0x0BC1, 0x0000, 0xD3},
   {0x0BAE, 0x0BC2, 0x0000, 0xE3},
   {0x0BAF, 0x0000, 0x0000, 0xC2},
   {0x0BAF, 0x0B82, 0x0000, 0xF6},
   {0x0BAF, 0x0BC1, 0x0000, 0xD4},
   {0x0BAF, 0x0BC2, 0x0000, 0xE4},
   {0x0BB0, 0x0000, 0x0000, 0xC3},
   {0x0BB0, 0x0B82, 0x0000, 0xF7},
   {0x0BB0, 0x0BC1, 0x0000, 0xD5},
   {0x0BB0, 0x0BC2, 0x0000, 0xE5},
   {0x0BB1, 0x0000, 0x0000, 0xC8},
   {0x0BB1, 0x0B82, 0x0000, 0xFC},
   {0x0BB1, 0x0BC1, 0x0000, 0xDA},
   {0x0BB1, 0x0BC2, 0x0000, 0xEA},
   {0x0BB2, 0x0000, 0x0000, 0xC4},
   {0x0BB2, 0x0B82, 0x0000, 0xF8},
   {0x0BB2, 0x0BC1, 0x0000, 0xD6},
   {0x0BB2, 0x0BC2, 0x0000, 0xE6},
   {0x0BB3, 0x0000, 0x0000, 0xC7},
   {0x0BB3, 0x0B82, 0x0000, 0xFB},
   {0x0BB3, 0x0BC1, 0x0000, 0xD9},
   {0x0BB3, 0x0BC2, 0x0000, 0xE9},
   {0x0BB4, 0x0000, 0x0000, 0xC6},
   {0x0BB4, 0x0B82, 0x0000, 0xFA},
   {0x0BB4, 0x0BC1, 0x0000, 0xD8},
   {0x0BB4, 0x0BC2, 0x0000, 0xE8},
   {0x0BB5, 0x0000, 0x0000, 0xC5},
   {0x0BB5, 0x0B82, 0x0000, 0xF9},
   {0x0BB5, 0x0BC1, 0x0000, 0xD7},
   {0x0BB5, 0x0BC2, 0x0000, 0xE7},
   {0x0BB7, 0x0000, 0x0000, 0x84},
   {0x0BB7, 0x0B82, 0x0000, 0x89},
   {0x0BB8, 0x0000, 0x0000, 0x85},
   {0x0BB8, 0x0B82, 0x0000, 0x8A},
   {0x0BB9, 0x0000, 0x0000, 0x86},
   {0x0BB9, 0x0B82, 0x0000, 0x8B},
   {0x0BBE, 0x0000, 0x0000, 0xA1},
   {0x0BBF, 0x0000, 0x0000, 0xA2},
   {0x0BC0, 0x0000, 0x0000, 0xA3},
   {0x0BC1, 0x0000, 0x0000, 0xA4},
   {0x0BC2, 0x0000, 0x0000, 0xA5},
   {0x0BC6, 0x0000, 0x0000, 0xA6},
   {0x0BC7, 0x0000, 0x0000, 0xA7},
   {0x0BC8, 0x0000, 0x0000, 0xA8},
   {0x0BCC, 0x0000, 0x0000, 0xAA},
   {0x0BE6, 0x0000, 0x0000, 0x80},
   {0x0BE7, 0x0000, 0x0000, 0x81},
   {0x0BE7, 0x0BB7, 0x0000, 0x87},
   {0x0BE7, 0x0BB7, 0x0B82, 0x8C},
   {0x0BE8, 0x0000, 0x0000, 0x8D},
   {0x0BE9, 0x0000, 0x0000, 0x8E},
   {0x0BEA, 0x0000, 0x0000, 0x8F},
   {0x0BEB, 0x0000, 0x0000, 0x90},
   {0x0BEC, 0x0000, 0x0000, 0x95},
   {0x0BED, 0x0000, 0x0000, 0x96},
   {0x0BEE, 0x0000, 0x0000, 0x97},
   {0x0BEF, 0x0000, 0x0000, 0x98},
   {0x0BF0, 0x0000, 0x0000, 0x9D},
   {0x0BF1, 0x0000, 0x0000, 0x9E},
   {0x0BF2, 0x0000, 0x0000, 0x9F},
   {0x2018, 0x0000, 0x0000, 0x91},
   {0x2019, 0x0000, 0x0000, 0x92},
   {0x201C, 0x0000, 0x0000, 0x93},
   {0x201C, 0x0000, 0x0000, 0x94}
};

static const ushort sg_TsToUn [][3] = {
   // Starting at 0x80
   {0x0BE6, 0x0000, 0x0000},
   {0x0BE7, 0x0000, 0x0000},
   {0x0000, 0x0000, 0x0000}, // unknown
   {0x0B9C, 0x0000, 0x0000},
   {0x0BB7, 0x0000, 0x0000},
   {0x0BB8, 0x0000, 0x0000},
   {0x0BB9, 0x0000, 0x0000},
   {0x0BE7, 0x0BB7, 0x0000},
   {0x0B9C, 0x0B82, 0x0000},
   {0x0BB7, 0x0B82, 0x0000},
   {0x0BB8, 0x0B82, 0x0000},
   {0x0BB9, 0x0B82, 0x0000},
   {0x0BE7, 0x0BB7, 0x0B82},
   {0x0BE8, 0x0000, 0x0000},
   {0x0BE9, 0x0000, 0x0000},
   {0x0BEA, 0x0000, 0x0000},
   {0x0BEB, 0x0000, 0x0000},
   {0x2018, 0x0000, 0x0000},
   {0x2019, 0x0000, 0x0000},
   {0x201C, 0x0000, 0x0000},
   {0x201C, 0x0000, 0x0000}, // two of the same??
   {0x0BEC, 0x0000, 0x0000},
   {0x0BED, 0x0000, 0x0000},
   {0x0BEE, 0x0000, 0x0000},
   {0x0BEF, 0x0000, 0x0000},
   {0x0B99, 0x0BC1, 0x0000},
   {0x0B9E, 0x0BC1, 0x0000},
   {0x0B99, 0x0BC2, 0x0000},
   {0x0B9E, 0x0BC2, 0x0000},
   {0x0BF0, 0x0000, 0x0000},
   {0x0BF1, 0x0000, 0x0000},
   {0x0BF2, 0x0000, 0x0000},
   {0x00A0, 0x0000, 0x0000},
   {0x0BBE, 0x0000, 0x0000},
   {0x0BBF, 0x0000, 0x0000},
   {0x0BC0, 0x0000, 0x0000},
   {0x0BC1, 0x0000, 0x0000},
   {0x0BC2, 0x0000, 0x0000},
   {0x0BC6, 0x0000, 0x0000},
   {0x0BC7, 0x0000, 0x0000},
   {0x0BC8, 0x0000, 0x0000},
   {0x00A9, 0x0000, 0x0000},
   {0x0BCC, 0x0000, 0x0000},
   {0x0B85, 0x0000, 0x0000},
   {0x0B86, 0x0000, 0x0000},
   {0x0B87, 0x0000, 0x0000},
   {0x0B88, 0x0000, 0x0000},
   {0x0B89, 0x0000, 0x0000},
   {0x0B8A, 0x0000, 0x0000},
   {0x0B8E, 0x0000, 0x0000},
   {0x0B8F, 0x0000, 0x0000},
   {0x0B90, 0x0000, 0x0000},
   {0x0B92, 0x0000, 0x0000},
   {0x0B93, 0x0000, 0x0000},
   {0x0B94, 0x0000, 0x0000},
   {0x0B83, 0x0000, 0x0000},
   {0x0B95, 0x0000, 0x0000},
   {0x0B99, 0x0000, 0x0000},
   {0x0B9A, 0x0000, 0x0000},
   {0x0B9E, 0x0000, 0x0000},
   {0x0B9F, 0x0000, 0x0000},
   {0x0BA3, 0x0000, 0x0000},
   {0x0BA4, 0x0000, 0x0000},
   {0x0BA8, 0x0000, 0x0000},
   {0x0BAA, 0x0000, 0x0000},
   {0x0BAE, 0x0000, 0x0000},
   {0x0BAF, 0x0000, 0x0000},
   {0x0BB0, 0x0000, 0x0000},
   {0x0BB2, 0x0000, 0x0000},
   {0x0BB5, 0x0000, 0x0000},
   {0x0BB4, 0x0000, 0x0000},
   {0x0BB3, 0x0000, 0x0000},
   {0x0BB1, 0x0000, 0x0000},
   {0x0BA9, 0x0000, 0x0000},
   {0x0B9F, 0x0BBF, 0x0000},
   {0x0B9F, 0x0BC0, 0x0000},
   {0x0B95, 0x0BC1, 0x0000},
   {0x0B9A, 0x0BC1, 0x0000},
   {0x0B9F, 0x0BC1, 0x0000},
   {0x0BA3, 0x0BC1, 0x0000},
   {0x0BA4, 0x0BC1, 0x0000},
   {0x0BA8, 0x0BC1, 0x0000},
   {0x0BAA, 0x0BC1, 0x0000},
   {0x0BAE, 0x0BC1, 0x0000},
   {0x0BAF, 0x0BC1, 0x0000},
   {0x0BB0, 0x0BC1, 0x0000},
   {0x0BB2, 0x0BC1, 0x0000},
   {0x0BB5, 0x0BC1, 0x0000},
   {0x0BB4, 0x0BC1, 0x0000},
   {0x0BB3, 0x0BC1, 0x0000},
   {0x0BB1, 0x0BC1, 0x0000},
   {0x0BA9, 0x0BC1, 0x0000},
   {0x0B95, 0x0BC2, 0x0000},
   {0x0B9A, 0x0BC2, 0x0000},
   {0x0B9F, 0x0BC2, 0x0000},
   {0x0BA3, 0x0BC2, 0x0000},
   {0x0BA4, 0x0BC2, 0x0000},
   {0x0BA8, 0x0BC2, 0x0000},
   {0x0BAA, 0x0BC2, 0x0000},
   {0x0BAE, 0x0BC2, 0x0000},
   {0x0BAF, 0x0BC2, 0x0000},
   {0x0BB0, 0x0BC2, 0x0000},
   {0x0BB2, 0x0BC2, 0x0000},
   {0x0BB5, 0x0BC2, 0x0000},
   {0x0BB4, 0x0BC2, 0x0000},
   {0x0BB3, 0x0BC2, 0x0000},
   {0x0BB1, 0x0BC2, 0x0000},
   {0x0BA9, 0x0BC2, 0x0000},
   {0x0B95, 0x0B82, 0x0000},
   {0x0B99, 0x0B82, 0x0000},
   {0x0B9A, 0x0B82, 0x0000},
   {0x0B9E, 0x0B82, 0x0000},
   {0x0B9F, 0x0B82, 0x0000},
   {0x0BA3, 0x0B82, 0x0000},
   {0x0BA1, 0x0B82, 0x0000},
   {0x0BA8, 0x0B82, 0x0000},
   {0x0BAA, 0x0B82, 0x0000},
   {0x0BAE, 0x0B82, 0x0000},
   {0x0BAF, 0x0B82, 0x0000},
   {0x0BB0, 0x0B82, 0x0000},
   {0x0BB2, 0x0B82, 0x0000},
   {0x0BB5, 0x0B82, 0x0000},
   {0x0BB4, 0x0B82, 0x0000},
   {0x0BB3, 0x0B82, 0x0000},
   {0x0BB1, 0x0B82, 0x0000},
   {0x0BA9, 0x0B82, 0x0000}
};

static int cmp(const ushort *s1, const ushort *s2, size_t len)
{
   int diff = 0;
   
   while (len-- && (diff = *s1++ - *s2++) == 0)
      ;
   
   return diff;
}

namespace {
unsigned char pdk_unicode_to_tscii(ushort u1, ushort u2, ushort u3)
{
   ushort s[3];
   s[0] = u1;
   s[1] = u2;
   s[2] = u3;
   
   int a = 0;  // start pos
   int b = sg_UnToTsLast; // end pos
   
   // do a binary search for the composed unicode in the list
   while (a <= b) {
      int w = (a + b) / 2;
      int j = cmp(sg_UnToTs[w], s, 3);
      
      if (j == 0) {
         // found it
         return sg_UnToTs[w][3];
      }
      if (j < 0) {
         a = w + 1;
      } else {
         b = w - 1;
      }
   }
   
   return 0;
}

unsigned int pdk_tscii_to_unicode(uint code, uint *s)
{
   int len = 0;
   for (int i = 0; i < 3; i++) {
      uint u = sg_TsToUn[code & 0x7f][i];
      s[i] = u;
      if (s[i]) len = i + 1;
   }
   
   return len;
}
} // anonymous namespace

} // internal
} // codecs
} // text
} // pdk