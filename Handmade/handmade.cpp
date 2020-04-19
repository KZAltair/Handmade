#include "Handmade.h"

internal void GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{
	local_persist real32 tSine;
	int16 ToneVolume = 1000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
	int16* SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
	{
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2.0f * Pi32 * 1.0f/(real32)WavePeriod;
	}
}

internal void
RenderGradient(game_offscreen_buffer* Buffer, int xOffset, int yOffset)
{
	//Fill in the pixels

	uint8* Row = (uint8*)Buffer->Memory;
	for (int y = 0; y < Buffer->Height; ++y)
	{
		uint32* Pixel = (uint32*)Row;
		for (int x = 0; x < Buffer->Width; ++x)
		{
			uint8 Blue = (x + xOffset);
			uint8 Green = (y + yOffset);
			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += Buffer->Pitch;
	}
}

internal void GameUpdateAndRender(game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state* GameState = (game_state*)Memory->PermanentStorage;
	if (!Memory->isInitialized)
	{
		char FileName[] = "test.bmp";

		debug_read_file_result File = DEBUGPlatformReadEntireFile(FileName);
		if (File.Contents)
		{
			DEBUGPlatformFreeFileMemory(File.Contents);
		}
		

		GameState->ToneHz = 256;
		Memory->isInitialized = true;
	}
	
	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers);
		++ControllerIndex)
	{
		game_controller_input* Controller = &Input->Controllers[ControllerIndex];
		if (Controller->IsAnalog)
		{
			//Use analog movement
			GameState->ToneHz = 256 + (int)(128.0f * (Controller->StickAverageY));
			GameState->BlueOffset += (int)4.0f * (int)(Controller->StickAverageX);
		}
		else
		{
			//Use digital tuning
			if (Controller->MoveLeft.EndedDown)
			{
				GameState->BlueOffset--;
			}
			if (Controller->MoveRight.EndedDown)
			{
				GameState->BlueOffset++;
			}
		}
		if (Controller->ActionDown.EndedDown)
		{
			GameState->GreenOffset += 1;
		}
	}




	RenderGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}

internal void GameGetSoundSamples(game_memory* Memory, game_sound_output_buffer* SoundBuffer)
{
	game_state* GameState = (game_state*)Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, GameState->ToneHz);
}