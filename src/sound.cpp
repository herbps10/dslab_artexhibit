#include "stdio.h"
#include "stdlib.h"
#include "SDL/SDL.h"
#include "SDL/SDL_mixer.h"

#define NUM_SOUNDS 6

int main()
{
	int grid_rows = 6;
	int grid_cols = 6;

	int *grid = (int *)malloc(sizeof(int) * grid_rows * grid_cols);

	int audio_rate = 22050;
	Uint16 audio_format = AUDIO_S16SYS;
	int audio_channels = 6;
	int audio_buffers = 4096;

	SDL_Surface *screen;
	Mix_Chunk *sounds[NUM_SOUNDS];
	int channel;

	for(int i = 0; i < NUM_SOUNDS; i++)
	{
		sounds[NUM_SOUNDS] = NULL;
	}

	// Initialize Grid
	for(int i = 0; i < grid_rows; i++)
	{
		for(int j = 0; j < grid_cols; j++)
		{
			grid[grid_cols * i + j] = 0;

			if(i == 1) { grid[grid_cols * i + j] = 1; }
			if(i == 2) { grid[grid_cols * i + j] = 1; }
		}
	}


	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
	}

	if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0)
	{
		printf("Unable to initialize audio: %s\n", Mix_GetError());
	}

	sounds[0] = Mix_LoadWAV("sounds/Piano.ff.C4.wav");
	sounds[1] = Mix_LoadWAV("sounds/Piano.ff.A4.wav");
	sounds[2] = Mix_LoadWAV("sounds/Piano.ff.D4.wav");
	sounds[3] = Mix_LoadWAV("sounds/Piano.ff.E4.wav");
	sounds[4] = Mix_LoadWAV("sounds/Piano.ff.G4.wav");
	sounds[5] = Mix_LoadWAV("sounds/Piano.ff.C5.wav");

	screen = SDL_SetVideoMode(320, 240, 0, SDL_ANYFORMAT);
	if(screen == NULL)
	{
		printf("Unable to set video mode: %s\n", SDL_GetError());
		return 1;
	}


	for(int i = 0; i < 4; i++)
	{
		for(int col = 0; col < grid_cols; col++)
		{
			for(int row = 0; row < grid_rows; row++)
			{
				if(grid[grid_cols * row + col] == 1)
				{
					Mix_PlayChannel(-1, sounds[row], 0);
					printf("Playing %i %i\n", row, col);
				}
			}

			SDL_Delay(300);
		}
	}


	SDL_Delay(1000);

	for(int i = 0; i < NUM_SOUNDS; i++)
	{
		Mix_FreeChunk(sounds[i]);
	}

	Mix_CloseAudio();
	SDL_Quit();

	return 0;
}
