#!/bin/sh
#
# Copyright (c) 2006 Claes Nästén <me@pekdon.net>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

#
# Script to generate configure script with the help of autotools.
#

# As which and whereis behave different on various platforms, a simple
# shell routine searching the split PATH is used instead.

# Do this outside the routine for effiency
PATH_SPLIT=`echo $PATH | awk -F ':' '{ for (i = 1; i < NF; i++) print $i}'`

# Search for command in PATH_SPLIT, sets command_found
find_command()
{
  command_found=''
  for dir in $PATH_SPLIT; do
    if [ -x "$dir/$1" ]; then
      command_found="$dir/$1"
      break
    fi
  done
}

# Search fod command, print $1 and run command with arg
find_fallback_and_execute()
{
 find_command "$2"
 if [ -z "$command_found" ]; then
   command_found="$3"
 fi

 printf " $1"
 $command_found $4

 if [ $? -ne 0 ]; then
     printf "\nfailed to execute %s, aborting!\n" $command_found
     exit 1
 fi
}

# Begin output
echo "Generating build scripts, this might take a while."

find_fallback_and_execute "aclocal" "aclocal" "aclocal-1.11" ""
find_fallback_and_execute "autoheader" "autoheader" "autoheader-2.65" ""
find_fallback_and_execute "autoconf" "autoconf" "autoconf-2.65" ""
find_fallback_and_execute "automake" "automake" "automake-1.11" "-a"

# End output
echo ""
echo "Done generating build scripts."
