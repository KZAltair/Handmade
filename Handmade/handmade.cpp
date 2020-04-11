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
			uint8 Blue = x + xOffset;
			uint8 Green = y + yOffset;
			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += Buffer->Pitch;
	}
}

internal void GameUpdateAndRender(game_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer,
									int ToneHz)
{
	int xOffset = 0;
	int yOffset = 0;
	GameOutputSound(SoundBuffer, ToneHz);
	RenderGradient(Buffer, xOffset, yOffset);
}