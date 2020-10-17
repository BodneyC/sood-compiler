all: parser

CC = g++

LLVMCONFIG = llvm-config
CPPFLAGS   = `$(LLVMCONFIG) --cppflags` -std=c++14
LDFLAGS    = `$(LLVMCONFIG) --ldflags`  -lpthread -ldl -lz -lncurses -rdynamic
LIBS       = `$(LLVMCONFIG) --libs`

XFLAGS = $(if $(DEBUG),-g,)

OBJS = parser.o tokens.o ast.o codegen.o
OBJS += $(if $(MAIN),$(MAIN),main.o)

parser.cpp: parser.y
	bison -d -o $@ $<

lexer.cpp: tokens.l
	flex -o $@ $< parser.hpp

%.o: %.cpp
	$(CC) $(XFLAGS) -c $(CPPFLAGS) -o $@ $<

parser: $(OBJS)
	$(CC) $(XFLAGS) $(LIBS) $(LDFLAGS) -o $@ $^

# --------- Additional --------- #

clean:
	$(RM) -rf parser.cpp parser.hpp parser *.o tokens.cpp

flex: lexer.cpp

bison: parser.cpp
