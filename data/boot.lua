-- define script values for the game here
-- things to scriptify:
-- input mappings
-- chunk generation
-- gameplay code

print("Booting the game...")

game = {
   day_length = 1200;
   fast_day_length = 5;
}

player = {
   accel = 30;
   friction = 0.05;
   gravity = -10;
}

vsync("off")
