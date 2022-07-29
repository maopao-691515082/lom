.PHONY: all clean

all:
	@mkdir -p build/tmpobjs build/lom/lib
	@rm -rf build/lom/include
	@cp -r include build/lom/include
	@python2 gen_build_mk.py
	@make -C build all

clean:
	@rm -rf build
