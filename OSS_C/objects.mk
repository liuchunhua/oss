OBJ := $(SRC:.c=.o)
LIBS := -lssl -lcrypto -lcurl -lxml2 -L../ -loss