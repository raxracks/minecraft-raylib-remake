rm -rf game
gcc game.c cJSON.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lwebsockets -o game && ./game
