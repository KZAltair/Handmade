#if !defined(HANDMADE_H)

struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;

};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16* Samples;
};

internal void GameUpdateAndRender(game_offscreen_buffer* Buffer, game_sound_output_buffer* SoundBuffer, int ToneHz);

#define HANDMADE_H
#endif
