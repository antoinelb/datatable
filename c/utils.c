//------------------------------------------------------------------------------
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// © H2O.ai 2018
//------------------------------------------------------------------------------
#include "utils.h"
#include <stdint.h>


int64_t min(int64_t a, int64_t b) { return a < b? a : b; }
int64_t max(int64_t a, int64_t b) { return a > b? a : b; }
float max_f4(float a, float b) { return a < b? b : a; }


namespace dt {

/**
 * Number of leading zeros.
 * This algorithm tuned for the case when the number of leading zeros is
 * expected to be large.
 * Algorithm 5-7 from Henry S. Warren "Hacker's Delight"
 */
template <typename T>
int nlz(T x) {
  T y;
  int n = sizeof(T) * 8;
  if (sizeof(T) >= 8) {
    y = x >> 32; if (y != 0) { n = n -32;  x = y; }
  }
  if (sizeof(T) >= 4) {
    y = x >> 16; if (y != 0) { n = n -16;  x = y; }
  }
  if (sizeof(T) >= 2) {
    y = x >> 8;  if (y != 0) { n = n - 8;  x = y; }
  }
  if (sizeof(T) >= 1) {
    y = x >> 4;  if (y != 0) { n = n - 4;  x = y; }
    y = x >> 2;  if (y != 0) { n = n - 2;  x = y; }
    y = x >> 1;  if (y != 0) return n - 2;
  }
  return n - static_cast<int>(x);
}

template int nlz(uint64_t);
template int nlz(uint32_t);
template int nlz(uint16_t);
template int nlz(uint8_t);

};  // namespace dt


/**
 * Fill the array `ptr` with `count` values `value` (the value is given as a
 * pointer of size `sz`). As a special case, if `value` pointer is NULL, then
 * fill with 0xFF bytes instead.
 * This is used for filling the columns with NAs.
 */
void set_value(void * __restrict__ ptr, const void * __restrict__ value,
               size_t sz, size_t count)
{
  if (count == 0) return;
  if (value == nullptr) {
    *(unsigned char *)ptr = 0xFF;
    count *= sz;
    sz = 1;
  } else {
    memcpy(ptr, value, sz);
  }
  size_t final_sz = sz * count;
  for (size_t i = sz; i < final_sz; i <<= 1) {
    size_t writesz = i < final_sz - i ? i : final_sz - i;
    memcpy(add_ptr(ptr, i), ptr, writesz);
  }
}


/**
 * Return current value on the system timer, in seconds. This function is
 * most suitable for profiling a piece of code: difference between two
 * consecutive calls to `wallclock()` will give the time elapsed in seconds.
 */
#if defined(CLOCK_REALTIME) && !defined(DISABLE_CLOCK_REALTIME)
  #include <time.h>
  double wallclock(void) {
    struct timespec tp;
    int ret = clock_gettime(CLOCK_REALTIME, &tp);
    return ret == 0? 1.0 * tp.tv_sec + 1e-9 * tp.tv_nsec : 0;
  }
#else
  #include <sys/time.h>
  double wallclock(void) {
    struct timeval tv;
    int ret = gettimeofday(&tv, nullptr);
    return ret == 0? 1.0 * tv.tv_sec + 1e-6 * tv.tv_usec : 0;
  }
#endif


/**
 * Helper function to print file's size in human-readable format. This will
 * produce strings such as:
 *     44.74GB (48043231704 bytes)
 *     921MB (965757797 bytes)
 *     2.206MB (2313045 bytes)
 *     38.69KB (39615 bytes)
 *     214 bytes
 *     0 bytes
 * The function returns a pointer to a static string buffer, so the caller
 * should not attempt to deallocate the buffer, or call this function from
 * multiple threads at the same time, or hold on to the value returned for
 * extended periods of time.
 */
const char* filesize_to_str(size_t fsize)
{
  #define NSUFFIXES 5
  #define BUFFSIZE 100
  static char suffixes[NSUFFIXES] = {'P', 'T', 'G', 'M', 'K'};
  static char output[BUFFSIZE];
  static const char one_byte[] = "1 byte";
  llu lsize = (llu) fsize;
  for (int i = 0; i <= NSUFFIXES; i++) {
    int shift = (NSUFFIXES - i) * 10;
    if ((fsize >> shift) == 0) continue;
    int ndigits = 3;
    for (; ndigits >= 1; ndigits--) {
      if ((fsize >> (shift + 12 - ndigits * 3)) == 0) break;
    }
    if (ndigits == 0 || (fsize == (fsize >> shift << shift))) {
      if (i < NSUFFIXES) {
        snprintf(output, BUFFSIZE, "%llu%cB",
                 lsize >> shift, suffixes[i]);
        return output;
      }
    } else {
      snprintf(output, BUFFSIZE, "%.*f%cB",
               ndigits, (double)fsize / (1 << shift), suffixes[i]);
      return output;
    }
  }
  if (fsize == 1) return one_byte;
  snprintf(output, BUFFSIZE, "%llu bytes", lsize);
  return output;
}


const char* humanize_number(size_t num) {
  static char outputs[270];
  static int curr_out = 0;
  char* output = outputs + (curr_out++) * 27;
  if (curr_out == 10) curr_out = 0;
  int len = 0;
  if (num) {
    while (true) {
      output[len++] = '0' + static_cast<char>(num % 10);
      num /= 10;
      if (!num) break;
      if (len % 4 == 3) output[len++] = ',';
    }
    for (int i = 0; i < len/2; ++i) {
      char c = output[i];
      output[i] = output[len - 1 - i];
      output[len - 1 - i] = c;
    }
  } else {
    output[len++] = '0';
  }
  output[len] = '\0';
  return output;
}
