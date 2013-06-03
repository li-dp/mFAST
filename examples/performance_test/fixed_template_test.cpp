// Copyright (c) 2013, Huang-Ming Huang,  Object Computing, Inc.
// All rights reserved.
//
// This file is part of mFAST.
//
//     mFAST is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     mFAST is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with mFast.  If not, see <http://www.gnu.org/licenses/>.
//
#include <mfast.h>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <limits>
#include <vector>
#include "example.h"

#include <boost/date_time/microsec_time_clock.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

const char usage[] =
  "  -f file     : FAST Message file (required)\n"
  "  -head n     : process only the first 'n' messages\n"
  "  -c count    : repeat the test 'count' times\n"
  "  -r          : Toggle 'reset decoder on every message' (default false).\n"
  "  -hfix n     : Skip n byte header before each message\n\n";


int read_file(const char* filename, std::vector<char>& contents)
{
  std::FILE*fp = std::fopen(filename, "rb");
  if (fp)
  {
    std::fseek(fp, 0, SEEK_END);
    contents.resize(std::ftell(fp));
    std::rewind(fp);
    std::fread(&contents[0], 1, contents.size(), fp);
    std::fclose(fp);
    return 0;
  }
  std::cerr << "File read error : " << filename << "\n";
  return -1;
}

int main(int argc, const char** argv)
{
  std::vector<char> message_contents;
  std::size_t head_n = std::numeric_limits<std::size_t>::max();
  std::size_t repeat_count = 1;
  bool force_reset = false;
  std::size_t skip_header_bytes = 0;

  int i = 1;
  int parse_status = 0;
  while (i < argc && parse_status == 0) {
    const char* arg = argv[i++];

    if (std::strcmp(arg, "-f") == 0) {
      parse_status = read_file(argv[i++], message_contents);
    }
    else if (std::strcmp(arg, "-head") == 0) {
      head_n = atoi(argv[i++]);
      if (head_n == 0) {
        std::cerr << "Invalid argument for '-head'\n";
        parse_status = -1;
      }
    }
    else if (std::strcmp(arg, "-c") == 0) {
      repeat_count = atoi(argv[i++]);
      if (repeat_count == 0) {
        std::cerr << "Invalid argument for '-c'\n";
        parse_status = -1;
      }
    }
    else if (std::strcmp(arg, "-r") == 0) {
      force_reset = true;
    }
    else if (std::strcmp(arg, "-hfix") == 0) {
      skip_header_bytes = atoi(argv[i++]);
    }
  }

  if (parse_status != 0 || message_contents.size() == 0) {
    std::cout << '\n' << usage;
    return -1;
  }

  try {

    mfast::arena_allocator alloc;
    mfast::decoder coder(alloc);
    const mfast::templates_description* descriptions[] = { &example::the_description };

    coder.include(descriptions);
    boost::posix_time::ptime start = boost::posix_time::microsec_clock::universal_time();
    {

      for (int j = 0; j < repeat_count; ++j) {

        mfast::fast_istream strm(&message_contents[0], message_contents.size());
        for (int i = 0; i < head_n && !strm.eof(); ++i) {
          strm.gbump(skip_header_bytes);
          if (!strm.eof()) {
            mfast::message_cref ref = coder.decode(strm, force_reset || (i == 0) );
          }
        }
      }
    }
    boost::posix_time::ptime stop = boost::posix_time::microsec_clock::universal_time();
    std::cout << "time spent " <<  static_cast<unsigned long>((stop - start).total_milliseconds()) << " msec\n";
  }
  catch (boost::exception& e) {
    std::cerr << boost::diagnostic_information(e);
    return -1;
  }

  return 0;
}