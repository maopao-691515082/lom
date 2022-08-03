.PHONY: all clean

all:
	@mkdir -p build/tmpobjs build/lom/lib
	@rm -rf build/lom/include
	@cp -r include build/lom/include
	@rm -rf build/lom/Make.def
	@cp Make.def build/lom/Make.def
	@python gen_build_mk.py
	@make -C build all

clean:
	@rm -rf build
