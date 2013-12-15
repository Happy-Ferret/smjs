# smjs - gypified spidermonkey

## Prerequisites

    * Linux, OS X or Windows
    * gcc 4.x or msvc 2010
    * [gyp](https://github.com/bnoordhuis/gyp) (make sure it's on your path)

## How to build on UNIX

    ./configure
    make -j <num-cpus> V=1 BUILDTYPE=Release

## How to build on Windows

    vcbuild.bat

## Maintainer's note

Here is how the directories map onto mozilla-central.

    mozilla-central                   local
    ---------------                   -----
    js/public/                     => include/js/
    intl/icu/source/common/unicode => include/unicode/
    js/src/                        => src/js/
    js/jsd/                        => src/jsd/
    mfbt/                          => src/mozilla/
