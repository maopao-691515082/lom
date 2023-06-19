.PHONY: all clean

all: clean
	@mkdir -p build/tmpobjs build/lom/lib build/test
	@rm -rf build/lom/include
	@cp -r include build/lom/include
	@rm -rf build/lom/defs.mk
	@cp defs.mk build/lom/defs.mk
	@python3 gen_build_mk.py
	@make -C build all

clean:
	@rm -rf build
