#coding=utf8

import os

os.chdir("src")

dirs = set()
files = []
for d, _, fs in os.walk("."):
    for f in fs:
        if f.endswith(".cpp"):
            if d != ".":
                dirs.add(d)
            files.append(("" if d == "." else d + "/") + f)

os.chdir("../build")

objs = []
for f in files:
    assert f.endswith(".cpp")
    objs.append("tmpobjs/" + f[: -3] + "o")

for i, f in enumerate(files):
    files[i] = "../src/" + f

mkf = open("Makefile", "w")
print >> mkf, """
.PHONY: all

all:"""

for d in dirs:
    print >> mkf, "\t@mkdir -p tmpobjs/%s" % d

CXX_FLAGS = [
    "-ggdb3",
    "-O2",
    "-Werror",
    "-Wshadow",
    "-fPIC",
    "-fno-strict-aliasing",
    "-fwrapv",
    "-pthread",
    "-U_FORTIFY_SOURCE",
]

for f, o in zip(files, objs):
    print >> mkf, "\tg++ %s -I../include -c -o %s %s" % (" ".join(CXX_FLAGS), o, f)

print >> mkf, "\tar -qs lom/lib/liblom.a %s" % " ".join(objs)
