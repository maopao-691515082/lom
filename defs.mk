LOM_CXX := g++
LOM_CXX_FLAGS := -std=gnu++17 -ggdb3 -O2 \
	-Werror -Wall -Wextra -Wshadow \
	-fPIC -pthread \
	-fno-strict-aliasing -fno-delete-null-pointer-checks -fno-strict-overflow -fsigned-char \
	-U_FORTIFY_SOURCE

LOM_AR := ar
LOM_AR_FLAGS := -crsP
