# Terminal-Pong
A pong game vs a CPU that runs in a Linux terminal!

As of version 1.0.0 there is only a play vs CPU mode, might add local play on same machine later. all the controls
are stated in game.

You also might want to comment out the "y_interval = START_Y_INTERVAL;" line in the new_game() function. It would
add more variance to how rounds start, may be more fun depending on how it feels for you.

TO BUILD THE GAME AFTER MAKING CHANGES:
gcc pong.c -o pong

TO RUN THE GAME:
./pong
