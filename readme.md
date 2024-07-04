Build instructions.
===================

1. Open a terminal.
2. Compile `nobs.c` (`cc nobs.c -o nobs` on Unix, `cl nobs.c` on Windows).
3. Run `nobs` with the path to [raylib](https://github.com/raysan5/raylib). Example: `nobs D:/Dev/Libs/raylib`

This will generate a `config.h` file containing the path to [raylib](https://github.com/raysan5/raylib).
You can now run `nobs` without any argument to rebuild `game_of_life`.
And if you need to change the path to [raylib](https://github.com/raysan5/raylib), you just need to edit `config.h` and run `nobs` again.
