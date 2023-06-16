LOM_CXX := g++
LOM_CXX_FLAGS := -std=gnu++17 -ggdb3 -O2 \
	-Werror -Wall -Wextra -Wshadow \
	-fPIC -pthread \
	-fno-strict-aliasing -fno-delete-null-pointer-checks -fno-strict-overflow -fsigned-char \
	-U_FORTIFY_SOURCE

LOM_AR := ar
LOM_AR_FLAGS := -crsP

LOM_LD := g++
LOM_LD_FLAGS := -rdynamic -pthread
LIM_LD_STD_LIB_FLAGS := -lrt -lm
