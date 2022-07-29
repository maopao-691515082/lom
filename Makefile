.PHONY: all clean

all:
	@mkdir -p build/tmpobjs build/lom/lib
	@python2 gen_build_mk.py
	@make -C build all
	@rm -rf build/lom/include
	@cp -r include build/lom/include

clean:
	@rm -rf build
