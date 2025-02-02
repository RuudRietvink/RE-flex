/******************************************************************************\
* Copyright (c) 2016, Robert van Engelen, Genivia Inc. All rights reserved.    *
*                                                                              *
* Redistribution and use in source and binary forms, with or without           *
* modification, are permitted provided that the following conditions are met:  *
*                                                                              *
*   (1) Redistributions of source code must retain the above copyright notice, *
*       this list of conditions and the following disclaimer.                  *
*                                                                              *
*   (2) Redistributions in binary form must reproduce the above copyright      *
*       notice, this list of conditions and the following disclaimer in the    *
*       documentation and/or other materials provided with the distribution.   *
*                                                                              *
*   (3) The name of the author may not be used to endorse or promote products  *
*       derived from this software without specific prior written permission.  *
*                                                                              *
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED *
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF         *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO   *
* EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,       *
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, *
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;  *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,     *
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR      *
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF       *
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                   *
\******************************************************************************/

/**
@file      input.h
@brief     RE/flex input character sequence class
@author    Robert van Engelen - engelen@genivia.com
@copyright (c) 2015-2017, Robert van Engelen, Genivia Inc. All rights reserved.
@copyright (c) BSD-3 License - see LICENSE.txt
*/

#ifndef REFLEX_INPUT_H
#define REFLEX_INPUT_H

#include <reflex/utf8.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <string>

namespace reflex {

/// Input character sequence class for unified access to sources of input text.
/**
Description
-----------

The Input class unifies access to a source of input text that constitutes a
sequence of characters:

- An Input object is instantiated and (re)assigned a (new) source input: either
  a `char*` string, a `wchar_t*` wide string, a `std::string`, a
  `std::wstring`, a `FILE*` descriptor, or a `std::istream` object.

- When assigned a wide string source as input, the wide character content is
  automatically converted to an UTF-8 character sequence when reading with
  get().  Wide strings are UCS-2/UCS-4 and may contain UTF-16 surrogate pairs.

- When assigned a `FILE*` source as input, the file is checked for the presence
  of a UTF-8 or a UTF-16 BOM (Byte Order Mark). A UTF-8 BOM is ignored and will
  not appear on the input character stream (and size is adjusted by 3 bytes). A
  UTF-16 BOM is intepreted, resulting in the conversion of the file content
  automatically to an UTF-8 character sequence when reading the file with
  get(). Also, size() gives the content size in the number of UTF-8 bytes.

- An input object can be reassigned a new source of input for reading at any
  time.

- An input object obeys move semantics. That is, after assigning an input
  object to another, the former can no longer be used to read input. This
  prevents adding the overhead and complexity of file and stream duplication.

- `size_t Input::get(char *buf, size_t len);` reads source input and fills `buf`
  with up to `len` bytes, returning the number of bytes read or zero when a
  stream or file is bad or when EOF is reached.

- `size_t Input::size();` returns the number of ASCII/UTF-8 bytes available
  to read from the source input or zero (zero is also returned when the size is
  not determinable). Use this function only before reading input with get().
  Wide character strings and UTF-16 `FILE*` content is counted as the total
  number of UTF-8 bytes that will be produced by get(). The size of a
  `std::istream` cannot be determined.

- `bool Input::good();` returns true if the input is readable and has no
  EOF or error state.  Returns false on EOF or if an error condition is
  present.

- `bool Input::eof();` returns true if the input reached EOF. Note that
  good() == ! eof() for string source input only, since files and streams may
  have error conditions that prevent reading. That is, for files and streams
  eof() implies good() == false, but not vice versa. Thus, an error is
  diagnosed when the condition good() == false && eof() == false holds. Note
  that get(buf, len) == 0 && len > 0 implies good() == false.

- `class Input::streambbuf(const Input&)` creates a `std::istream` for the
  given `Input` object.

- Compile with `WITH_UTF8_UNRESTRICTED` to enable unrestricted UTF-8 beyond
  U+10FFFF, permitting lossless UTF-8 encoding of 32 bit words without limits.

Example
-------

The following example shows how to use the Input class to read a character
sequence in blocks from a `std::ifstream` to copy to stdout:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    std::ifstream ifs;
    ifs.open("input.h", std::ifstream::in);
    reflex::Input input(ifs);
    char buf[1024];
    size_t len;
    while ((len = input.get(buf, sizeof(buf))) > 0)
      fwrite(buf, 1, len, stdout);
    if (!input.eof())
      std::cerr << "An IO error occurred" << std::endl;
    ifs.close();
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Example
-------

The following example shows how to use the Input class to store the entire
content of a file in a temporary buffer:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    reflex::Input input(fopen("input.h", "r"));
    if (input.file() == NULL)
      abort();
    size_t len = input.size(); // file size (minus any leading UTF BOM)
    char *buf = new char[len];
    input.get(buf, len);
    if (!input.eof())
      std::cerr << "An IO error occurred" << std::endl;
    fwrite(buf, 1, len, stdout);
    delete[] buf;
    fclose(input.file());
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In the above, files with UTF-16 and UTF-32 content are converted to UTF-8 by
`get(buf, len)`.  Also, `size()` returns the total number of UTF-8 bytes to
copy in the buffer by `get(buf, len)`.  The size is computed depending on the
UTF-8/16/32 file content encoding, i.e. given a leading UTF BOM in the file.
This means that UTF-16/32 files are read twice, first internally with `size()`
and then again with get(buf, len)`.

Example
-------

The following example shows how to use the Input class to read a character
sequence in blocks from a file:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    reflex::Input input(fopen("input.h", "r"));
    char buf[1024];
    size_t len;
    while ((len = input.get(buf, sizeof(buf))) > 0)
      fwrite(buf, 1, len, stdout);
    fclose(input);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Example
-------

The following example shows how to use the Input class to echo characters one
by one from stdin, e.g. reading input from a tty:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    reflex::Input input(stdin);
    char c;
    while (input.get(&c, 1))
      fputc(c, stdout);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Or if you prefer to use an int character and check for EOF explicitly:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    reflex::Input input(stdin);
    int c;
    while ((c = input.get()) != EOF)
      fputc(c, stdout);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Example
-------

The following example shows how to use the Input class to read a character
sequence in blocks from a wide character string, converting it to UTF-8 to copy
to stdout:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    reflex::Input input(L"Copyright ©"); // © is unicode U+00A9 and UTF-8 C2 A9
    char buf[8];
    size_t len;
    while ((len = input.get(buf, sizeof(buf))) > 0)
      fwrite(buf, 1, len, stdout);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Example
-------

The following example shows how to use the Input class to convert a wide
character string to UTF-8:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    reflex::Input input(L"Copyright ©"); // © is unicode U+00A9 and UTF-8 C2 A9
    size_t len = input.size(); // size of UTF-8 string
    char *buf = new char[len + 1];
    input.get(buf, len);
    buf[len] = '\0'; // make \0-terminated
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Example
-------

The following example shows how to switch source inputs while reading input
byte by byte (use a buffer as shown in other examples to improve efficiency):

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    reflex::Input input = "Hello";
    std::string message;
    char c;
    while (input.get(&c, 1))
      message.append(c);
    input = L" world! To ∞ and beyond."; // switch input to a wide string
    while (input.get(&c, 1))
      message.append(c);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Example
-------

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    reflex::Input input(fopen("legacy.txt", "r"), reflex::Input::file_encoding::ebcdic);
    if (input.file() == NULL)
      abort();
    reflex::Input::streambuf streambuf(input);
    std::istream stream(&streambuf);
    std::string data;
    int c;
    while ((c = stream.get()) != EOF)
      data.append(c);
    fclose(input.file());
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
class Input {
 public:
  /// Common file_encoding constants type.
  typedef unsigned short file_encoding_type;
  /// Common file_encoding constants.
  struct file_encoding {
    static const file_encoding_type plain   =  0; ///< plain octets: 7-bit ASCII, 8-bit binary or UTF-8 without BOM detected
    static const file_encoding_type utf8    =  1; ///< UTF-8 with BOM detected
    static const file_encoding_type utf16be =  2; ///< UTF-16 big endian
    static const file_encoding_type utf16le =  3; ///< UTF-16 little endian
    static const file_encoding_type utf32be =  4; ///< UTF-32 big endian
    static const file_encoding_type utf32le =  5; ///< UTF-32 little endian
    static const file_encoding_type latin   =  6; ///< Basic Latin ASCII and Latin-1 supplement, ISO-8859-1
    static const file_encoding_type cp437   =  7; ///< CP 427
    static const file_encoding_type cp850   =  8; ///< CP 850
    static const file_encoding_type cp858   =  9; ///< CP 858
    static const file_encoding_type ebcdic  = 10; ///< EBCDIC
    static const file_encoding_type cp1250  = 11; ///< CP 1250
    static const file_encoding_type cp1251  = 12; ///< CP 1251
    static const file_encoding_type cp1252  = 13; ///< CP 1252
    static const file_encoding_type cp1253  = 14; ///< CP 1253
    static const file_encoding_type cp1254  = 15; ///< CP 1254
    static const file_encoding_type cp1255  = 16; ///< CP 1255
    static const file_encoding_type cp1256  = 17; ///< CP 1256
    static const file_encoding_type cp1257  = 18; ///< CP 1257
    static const file_encoding_type cp1258  = 19; ///< CP 1258
    static const file_encoding_type custom  = 20; ///< custom code page
  };
  /// Stream buffer for reflex::Input, derived from std::streambuf.
  class streambuf;
  /// Stream buffer for reflex::Input to read DOS files, replaces CRLF by LF, derived from std::streambuf.
  class dos_streambuf;
  /// Copy constructor (with intended "move semantics" as internal state is shared, should not rely on using the rhs after copying).
  Input(const Input& input) ///< an Input object to share state with (undefined behavior results from using both objects)
    :
      cstring_(input.cstring_),
      wstring_(input.wstring_),
      file_(input.file_),
      istream_(input.istream_),
      size_(input.size_),
      uidx_(input.uidx_),
      utfx_(input.utfx_),
      page_(input.page_)
  {
    std::memcpy(utf8_, input.utf8_, sizeof(utf8_));
  }
  /// Construct empty input character sequence.
  Input()
    :
      cstring_(NULL),
      wstring_(NULL),
      file_(NULL),
      istream_(NULL),
      size_(0)
  {
    init();
  }
  /// Construct input character sequence from a char* string
  Input(
      const char *cstring, ///< char string
      size_t size)         ///< length of the string
    :
      cstring_(cstring),
      wstring_(NULL),
      file_(NULL),
      istream_(NULL),
      size_(size)
  {
    init();
  }
  /// Construct input character sequence from a NUL-terminated string.
  Input(const char *cstring) ///< NUL-terminated char* string
    :
      cstring_(cstring),
      wstring_(NULL),
      file_(NULL),
      istream_(NULL),
      size_(cstring != NULL ? std::strlen(cstring) : 0)
  {
    init();
  }
  /// Construct input character sequence from a std::string.
  Input(const std::string& string) ///< input string
    :
      cstring_(string.c_str()),
      wstring_(NULL),
      file_(NULL),
      istream_(NULL),
      size_(string.size())
  {
    init();
  }
  /// Construct input character sequence from a pointer to a std::string.
  Input(const std::string *string) ///< input string
    :
      cstring_(string != NULL ? string->c_str() : NULL),
      wstring_(NULL),
      file_(NULL),
      istream_(NULL),
      size_(string != NULL ? string->size() : 0)
  {
    init();
  }
  /// Construct input character sequence from a NUL-terminated wide character string.
  Input(const wchar_t *wstring) ///< NUL-terminated wchar_t* input string
    :
      cstring_(NULL),
      wstring_(wstring),
      file_(NULL),
      istream_(NULL),
      size_(0)
  {
    init();
  }
  /// Construct input character sequence from a std::wstring (may contain UTF-16 surrogate pairs).
  Input(const std::wstring& wstring) ///< input wide string
    :
      cstring_(NULL),
      wstring_(wstring.c_str()),
      file_(NULL),
      istream_(NULL),
      size_(0)
  {
    init();
  }
  /// Construct input character sequence from a pointer to a std::wstring (may contain UTF-16 surrogate pairs).
  Input(const std::wstring *wstring) ///< input wide string
    :
      cstring_(NULL),
      wstring_(wstring != NULL ? wstring->c_str() : NULL),
      file_(NULL),
      istream_(NULL),
      size_(0)
  {
    init();
  }
  /// Construct input character sequence from an open FILE* file descriptor, supports UTF-8 conversion from UTF-16 and UTF-32, use stdin if file == NULL.
  Input(FILE *file) ///< input file
    :
      cstring_(NULL),
      wstring_(NULL),
      file_(file),
      istream_(NULL),
      size_(0)
  {
    init();
  }
  /// Construct input character sequence from an open FILE* file descriptor, supports UTF-8 conversion from UTF-16 and UTF-32, use stdin if file == NULL.
  Input(
      FILE                 *file,        ///< input file
      file_encoding_type    enc,         ///< file_encoding (when UTF BOM is not present)
      const unsigned short *page = NULL) ///< code page for file_encoding::custom
    :
      cstring_(NULL),
      wstring_(NULL),
      file_(file),
      istream_(NULL),
      size_(0)
  {
    init();
    if (file_encoding() == file_encoding::plain)
      file_encoding(enc, page);
  }
  /// Construct input character sequence from a std::istream.
  Input(std::istream& istream) ///< input stream
    :
      cstring_(NULL),
      wstring_(NULL),
      file_(NULL),
      istream_(&istream),
      size_(0)
  {
    init();
  }
  /// Construct input character sequence from a pointer to a std::istream, use stdin if istream == NULL.
  Input(std::istream *istream) ///< input stream
    :
      cstring_(NULL),
      wstring_(NULL),
      file_(NULL),
      istream_(istream),
      size_(0)
  {
    init();
  }
  /// Copy assignment operator
  Input& operator=(const Input& input)
  {
    cstring_ = input.cstring_;
    wstring_ = input.wstring_;
    file_ = input.file_;
    istream_ = input.istream_;
    size_ = input.size_;
    uidx_ = input.uidx_;
    utfx_ = input.utfx_;
    page_ = input.page_;
    std::memcpy(utf8_, input.utf8_, sizeof(utf8_));
    return *this;
  }
  /// Cast this Input object to a string, returns NULL when this Input is not a string.
  operator const char *()
    /// @returns remaining unbuffered part of a NUL-terminated string or NULL.
  {
    return cstring_;
  }
  /// Cast this Input object to a wide character string, returns NULL when this Input is not a wide string.
  operator const wchar_t *()
    /// @returns remaining unbuffered part of the NUL-terminated wide character string or NULL.
  {
    return wstring_;
  }
  /// Cast this Input object to a file descriptor FILE*, returns NULL when this Input is not a FILE*.
  operator FILE *()
    /// @returns pointer to current file descriptor or NULL.
  {
    return file_;
  }
  /// Cast this Input object to a std::istream*, returns NULL when this Input is not a std::istream.
  operator std::istream *()
    /// @returns pointer to current std::istream or NULL.
  {
    return istream_;
  }
  // Cast this Input object to bool, same as checking good().
  operator bool()
    /// @returns true if a non-empty sequence of characters is available to get.
  {
    return good();
  }
  /// Get the remaining string of this Input object, returns NULL when this Input is not a string.
  const char *cstring()
    /// @returns remaining unbuffered part of the NUL-terminated string or NULL.
  {
    return cstring_;
  }
  /// Get the remaining wide character string of this Input object, returns NULL when this Input is not a wide string.
  const wchar_t *wstring()
    /// @returns remaining unbuffered part of the NUL-terminated wide character string or NULL.
  {
    return wstring_;
  }
  /// Get the FILE* of this Input object, returns NULL when this Input is not a FILE*.
  FILE *file()
    /// @returns pointer to current file descriptor or NULL.
  {
    return file_;
  }
  /// Get the std::istream of this Input object, returns NULL when this Input is not a std::istream.
  std::istream *istream()
    /// @returns pointer to current std::istream or NULL.
  {
    return istream_;
  }
  /// Get the size of the input character sequence in number of ASCII/UTF-8 bytes (zero if size is not determinable from a `FILE*` or `std::istream` source).
  size_t size()
    /// @returns the nonzero number of ASCII/UTF-8 bytes available to read, or zero when source is empty or if size is not determinable e.g. when reading from standard input.
  {
    if (cstring_)
      return size_;
    if (wstring_)
    {
      if (size_ == 0)
        wstring_size();
    }
    else if (file_)
    {
      if (size_ == 0)
        file_size();
    }
    else if (istream_)
    {
      if (size_ == 0)
        istream_size();
    }
    return size_;
  }
  /// Check if this Input object was assigned a character sequence.
  bool assigned() const
    /// @returns true if this Input object was assigned (not default constructed or cleared).
  {
    return cstring_ || wstring_ || file_ || istream_;
  }
  /// Clear this Input by unassigning it.
  void clear()
  {
    cstring_ = NULL;
    wstring_ = NULL;
    file_ = NULL;
    istream_ = NULL;
    size_ = 0;
  }
  /// Check if input is available.
  bool good()
    /// @returns true if a non-empty sequence of characters is available to get.
  {
    if (cstring_)
      return size_ > 0;
    if (wstring_)
      return *wstring_ != L'\0';
    if (file_)
      return !::feof(file_) && !::ferror(file_);
    if (istream_)
      return istream_->good();
    return false;
  }
  /// Check if input reached EOF.
  bool eof()
    /// @returns true if input is at EOF and no characters are available.
  {
    if (cstring_)
      return size_ == 0;
    if (wstring_)
      return *wstring_ == L'\0';
    if (file_)
      return ::feof(file_) != 0;
    if (istream_)
      return istream_->eof();
    return true;
  }
  /// Get a single character (unsigned char 0..255) or EOF (-1) when end-of-input is reached.
  int get()
  {
    char c;
    if (get(&c, 1))
      return static_cast<unsigned char>(c);
    return EOF;
  }
  /// Copy character sequence data into buffer.
  size_t get(
      char  *s, ///< points to the string buffer to fill with input
      size_t n) ///< size of buffer pointed to by s
    /// @returns the nonzero number of (less or equal to n) 8-bit characters added to buffer s from the current input, or zero when EOF.
  {
    if (cstring_)
    {
      size_t k = size_;
      if (k > n)
        k = n;
      std::memcpy(s, cstring_, k);
      cstring_ += k;
      size_ -= k;
      return k;
    }
    if (wstring_)
    {
      size_t k = n;
      if (uidx_ < sizeof(utf8_))
      {
        size_t l = std::strlen(utf8_ + uidx_);
        if (l > k)
          l = k;
        std::memcpy(s, utf8_ + uidx_, l);
        uidx_ += static_cast<unsigned short>(l);
        k -= l;
        if (k == 0)
        {
          if (size_ >= n)
            size_ -= n;
          return n;
        }
        s += l;
        uidx_ = sizeof(utf8_);
      }
      wchar_t c;
      while ((c = *wstring_) != L'\0' && k > 0)
      {
        if (c < 0x80)
        {
          *s++ = static_cast<char>(c);
          --k;
        }
        else
        {
          size_t l;
          if (c >= 0xD800 && c < 0xE000)
          {
            // UTF-16 surrogate pair
            if (c < 0xDC00 && (wstring_[1] & 0xFC00) == 0xDC00)
              l = utf8(0x010000 - 0xDC00 + ((c - 0xD800) << 10) + *++wstring_, utf8_);
            else
              l = utf8(REFLEX_NONCHAR, utf8_);
          }
          else
          {
            l = utf8(c, utf8_);
          }
          if (k < l)
          {
            utf8_[l] = '\0';
            uidx_ = static_cast<unsigned short>(k);
            std::memcpy(s, utf8_, k);
            s += k;
            k = 0;
          }
          else
          {
            std::memcpy(s, utf8_, l);
            s += l;
            k -= l;
          }
        }
        ++wstring_;
      }
      if (size_ >= n - k)
        size_ -= n - k;
      return n - k;
    }
    if (file_)
    {
      size_t k = file_get(s, n);
      if (size_ >= k)
        size_ -= k;
      return k;
    }
    if (istream_)
    {
      size_t k = static_cast<size_t>(n == 1 ? istream_->get(s[0]).gcount() : istream_->read(s, static_cast<std::streamsize>(n)).gcount());
      if (size_ >= k)
        size_ -= k;
      return k;
    }
    return 0;
  }
  /// Set encoding for `FILE*` input.
  void file_encoding(
      file_encoding_type    enc,         ///< file_encoding
      const unsigned short *page = NULL) ///< custom code page for file_encoding::custom
    ;
  /// Get encoding of the current `FILE*` input.
  file_encoding_type file_encoding() const
    /// @returns current file_encoding constant
  {
    return utfx_;
  }
  /// Initialize the state after (re)setting the input source, auto-detects UTF BOM in FILE* input if the file size is known.
  void init()
  {
    std::memset(utf8_, 0, sizeof(utf8_));
    uidx_ = sizeof(utf8_);
    utfx_ = 0;
    page_ = NULL;
    if (file_)
      file_init();
  }
  /// Called by init() for a FILE*.
  void file_init();
  /// Called by size() for a wstring.
  void wstring_size();
  /// Called by size() for a FILE*.
  void file_size();
  /// Called by size() for a std::istream.
  void istream_size();
  /// Implements get() on a FILE*.
  size_t file_get(
      char  *s, ///< points to the string buffer to fill with input
      size_t n) ///< size of buffer pointed to by s
      ;
  const char           *cstring_; ///< char string input (when non-null) of length reflex::Input::size_
  const wchar_t        *wstring_; ///< NUL-terminated wide string input (when non-null)
  FILE                 *file_;    ///< FILE* input (when non-null)
  std::istream         *istream_; ///< stream input (when non-null)
  size_t                size_;    ///< size of the remaining input in bytes (size_ == 0 may indicate unset value)
  char                  utf8_[8]; ///< UTF-8 normalization buffer
  unsigned short        uidx_;    ///< index in utf8_[] or >= 8 when unused
  file_encoding_type    utfx_;    ///< file_encoding
  const unsigned short *page_;    ///< custom code page
};

/// Stream buffer for reflex::Input, derived from std::streambuf.
class Input::streambuf : public std::streambuf {
 public:
  streambuf(const reflex::Input& input)
    :
      input_(input),
      ch_(input_.get())
  { }
 private:
  virtual int_type underflow()
  {
    if (ch_ == EOF)
      return traits_type::eof();
    return traits_type::to_int_type(ch_);
  }
  virtual int_type uflow()
  {
    if (ch_ == EOF)
      return traits_type::eof();
    int c = ch_;
    ch_ = input_.get();
    return traits_type::to_int_type(c);
  }
  virtual std::streamsize showmanyc()
  {
    return input_.size();
  }
 protected:
  reflex::Input input_;
  int ch_;
};

/// Stream buffer for reflex::Input to read DOS files, replaces CRLF by LF, derived from std::streambuf.
class Input::dos_streambuf : public std::streambuf {
 public:
  dos_streambuf(const reflex::Input& input)
    :
      input_(input),
      ch1_(input_.get()),
      ch2_(EOF)
  { }
 private:
  virtual int_type underflow()
  {
    if (ch1_ == EOF)
      return traits_type::eof();
    if (ch1_ == '\r')
    {
      ch2_ = input_.get();
      if (ch2_ == '\n')
      {
        ch1_ = ch2_;
        ch2_ = EOF;
      }
    }
    return traits_type::to_int_type(ch1_);
  }
  virtual int_type uflow()
  {
    if (ch1_ == EOF)
      return traits_type::eof();
    int c = ch1_;
    if (ch2_ == EOF)
    {
      ch1_ = input_.get();
    }
    else
    {
      ch1_ = ch2_;
      ch2_ = EOF;
    }
    if (c == '\r' && ch1_ == '\n')
    {
      c = ch1_;
      ch1_ = input_.get();
    }
    return traits_type::to_int_type(c);
  }
  virtual std::streamsize showmanyc()
  {
    return input_.size();
  }
 protected:
  reflex::Input input_;
  int ch1_;
  int ch2_;
};

} // namespace reflex

#endif
