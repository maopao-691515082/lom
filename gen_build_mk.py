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

include lom/Make.def

all:"""

for d in dirs:
    print >> mkf, "\t@mkdir -p tmpobjs/%s" % d

for f, o in zip(files, objs):
    print >> mkf, "\t$(LOM_CXX) $(LOM_CXX_FLAGS) -Ilom/include -c -o %s %s" % (o, f)

print >> mkf, "\t$(LOM_AR) $(LOM_AR_FLAGS) lom/lib/liblom.a %s" % " ".join(objs)

mkf.close()
