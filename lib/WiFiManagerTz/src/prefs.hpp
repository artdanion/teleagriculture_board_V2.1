/*\
 *
 * WiFiManagerTz
 *
 * Copyright (c) 2022 tobozo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
\*/

#pragma once

#include <Arduino.h>

#define PREF_NAMESPACE "wm"


namespace WiFiManagerNS
{

  namespace prefs
  {
    void reset();
    void set( const char *name, const char *value, size_t len );
    void get( const char *name, char *dest, size_t max_len, const char *default_value );
    void setUChar( const char *name, uint8_t value );
    void getUChar( const char *name, uint8_t *dest, uint8_t default_value );
    void setFloat( const char *name, float value );
    void getFloat( const char *name, float *dest, float default_value );
    void setBool( const char* key, bool value);
    void getBool( const char *name, bool *dest, bool default_value );
    void setUInt( const char* name, unsigned int value);
    void getUInt( const char *name, unsigned int *dest, unsigned int default_value );

  };
}
