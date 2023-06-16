import os

os.chdir("src")

dirs = set()
files = []
for d, _, fs in os.walk("."):
    if d.startswith("./"):
        d = d[2 :]
    else:
        assert d == "."
    for f in fs:
        if f.endswith(".cpp"):
            if d != ".":
                dirs.add(d)
            files.append(("" if d == "." else d + "/") + f)

os.chdir("../test")

cases = os.listdir(".")

os.chdir("../build")

objs = []
for f in files:
    assert f.endswith(".cpp")
    objs.append("tmpobjs/" + f[: -3] + "o")

for i, f in enumerate(files):
    files[i] = "../src/" + f

mkf = open("Makefile", "w")
def write_line(line):
    mkf.write(line)
    mkf.write("\n")
write_line("""
.PHONY: all

include lom/defs.mk

all:""")

for d in dirs:
    write_line("\t@mkdir -p tmpobjs/%s" % d)

for f, o in zip(files, objs):
    write_line("\t$(LOM_CXX) $(LOM_CXX_FLAGS) -Ilom/include -c -o %s %s" % (o, f))

write_line("\t$(LOM_AR) $(LOM_AR_FLAGS) lom/lib/liblom.a %s" % " ".join(objs))

for case in cases:
    write_line(
        "\t$(LOM_LD) $(LOM_CXX_FLAGS) $(LOM_LD_FLAGS) "
        "-o test/%s ../test/%s/*.cpp lom/lib/liblom.a $(LIM_LD_STD_LIB_FLAGS)" %
        (case, case))

mkf.close()
