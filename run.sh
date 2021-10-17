# gcc app.c -o app -lpaho-mqtt3c -Wall && ./app
# gcc app.c -o app -lpaho-mqtt3cs -Wall && ./app
gcc app.c -ljson-c -o app -lpaho-mqtt3cs -Wall -I/usr/include/libbson-1.0 -I/usr/include/libmongoc-1.0 -lmongoc-1.0 -lbson-1.0 && ./app